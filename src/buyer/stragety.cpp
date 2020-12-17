#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <fstream>
#include <map>
#include <queue>
#include <random>
#include <set>

#include "buyer.h"
#include "src/comm/config.h"
#include "src/comm/debug.h"
#include "src/comm/log.h"
#include "src/comm/scope_time.h"
#include "src/comm/tools.h"
#include "src/seller/seller.h"

// int factor[6] = {0, 1, 3, 50, 300, 1000};  // 157.990
// int factor[6] = {0, 1, 5, 80, 300, 800};  // 157.971
// int factor[6] = {0, 1, 3, 33, 333, 3333};  // 157.995
int factor[6] = {0, 1, 6, 66, 666, 6666};  // 157.996

Depot* choice_best_depot_step(BGoods* buyer, int intent_id) {
    auto Seller = Seller::GetInstance();
    auto& depots = Seller->GetDepots();

    int max_value = 0;
    Depot* result = nullptr;

    for (auto& depot : depots) {
        int val = 0;
        for (auto& it : buyer->GetPermuIntents()) {
            if (intent_id == 0 && it->orders[0] != 0) continue;
            val += depot->map_stock[it->map_key] * it->score * factor[(it->values).size()];
        }
        if (val > max_value) {
            max_value = val;
            result = depot;
        }
    }

    return result;
}

void Buyer::do_business(Depot* depot, Depot::Item* item, BGoods* buyer, SGoods* seller) {
    auto Seller = Seller::GetInstance();
    auto& global_count = Seller->GetGloalCount();

    int val = min(buyer->GetBuyCount(), seller->GetGoodStock());
    buyer->SetBuyCount(buyer->GetBuyCount() - val);
    seller->SetGoodStock(seller->GetGoodStock() - val);
    item->tol_stock -= val;

    for (auto& it : Seller->GetPropotys()[seller->GetGoodID()]) {
        global_count[it->map_key] -= val;
        depot->map_stock[it->map_key] -= val;
    }

    Business bus(buyer, seller, val, {});
    m_results.push_back(bus);
}

void Buyer::assign_buyer(BGoods* buyer, int intent_id) {
    auto Seller = Seller::GetInstance();
    auto& global_count = Seller->GetGloalCount();
    if (intent_id == 0 && global_count[buyer->GetIntentMapKeys()[0]] <= 0) return;

    while (buyer->GetBuyCount() > 0) {
        // [step1] 选一个仓库
        auto depot = choice_best_depot_step(buyer, intent_id);
        if (depot == nullptr) {
            break;
        }

        // [step2] 意向排序
        auto& permu_intenters = buyer->GetPermuIntents();
        sort(permu_intenters.begin(), permu_intenters.end(), [&](const Intent* g1, const Intent* g2) {
            int count1 = depot->map_stock[g1->map_key], count2 = depot->map_stock[g2->map_key];
            if (g1->score == g2->score) {
                return count1 > count2;
            }
            return g1->score > g2->score;
        });

        // [step3] 得分从大到小枚举意向组合
        int do_count = 0;
        for (auto& intenter : permu_intenters) {
            if (buyer->GetBuyCount() <= 0) break;
            if (do_count++ > 1) {
                this->assign_special(buyer, intent_id);
            }
            if (buyer->GetBuyCount() <= 0) break;

            if (depot->map_stock[intenter->map_key] <= 0) continue;    // 库存不够
            if (intent_id == 0 && intenter->orders[0] != 0) continue;  // 意向不对

            auto& items = depot->map_sellers[intenter->map_key];
            sort(items.begin(), items.end(),
                 [&](const Depot::Item* it1, const Depot::Item* it2) { return it1->tol_stock > it2->tol_stock; });
            pop_depot(items);

            while (buyer->GetBuyCount() > 0 && !items.empty()) {
                auto& item = items.front();
                item->pop_front();

                // [step4] 当前货物编号挨个选
                while (buyer->GetBuyCount() > 0 && !item->values.empty()) {
                    auto& seller = item->values.front();
                    this->do_business(depot, item, buyer, seller);
                    item->pop_front();
                }
                pop_depot(items);
            }
        }
    }
}

void Buyer::assign_last_buyers() {
    vector<BGoods*> last_buyers;
    for (auto& buyer : m_goods) {
        if (buyer->GetBuyCount() > 0) {
            last_buyers.push_back(buyer);
        }
    }
    sort(last_buyers.begin(), last_buyers.end(),
         [&](const BGoods* g1, const BGoods* g2) { return g1->GetBuyCount() > g2->GetBuyCount(); });

    log_debug("第 last 轮, 买家总数: %d", last_buyers.size());

    auto Seller = Seller::GetInstance();

    int idx = 0;
    for (auto& buyer : last_buyers) {
        if (++idx % 1000 == 0) log_debug("* idx: %d", idx);
        for (auto& depot : Seller->GetDepots()) {
            auto& items = depot->sellers[buyer->GetBreed()];
            pop_depot(items);

            while (buyer->GetBuyCount() > 0 && !items.empty()) {
                auto& item = items.front();
                item->pop_front();
                while (buyer->GetBuyCount() > 0 && !item->values.empty()) {
                    auto& seller = item->values.front();
                    this->do_business(depot, item, buyer, seller);
                    item->pop_front();
                }
                pop_depot(items);
            }
        }
    }
}

vector<pair<string, deque<BGoods*>>> Buyer::create_buyers(int intent_id, int& buyers_count,
                                                          unordered_map<string, int>& ump_count) {
    unordered_map<string, deque<BGoods*>> ump;

    for (const auto& buyer : m_goods) {
        if (buyer->GetBuyCount() <= 0 || buyer->GetIntentMapKeys()[intent_id].empty()) {
            continue;
        }
        const string& key = buyer->GetIntentMapKeys()[intent_id];
        ump[key].push_back(buyer);
        ump_count[key] += buyer->GetBuyCount();
    }
    vector<pair<string, deque<BGoods*>>> ans;
    for (auto& [intent, buyers] : ump) {
        ans.push_back({intent, buyers});
    }

    buyers_count = 0;
    for (auto& it : ans) {
        auto& buyers = it.second;
        if (intent_id == 0) {
            sort(buyers.begin(), buyers.end(), [&](const BGoods* g1, const BGoods* g2) {
                if (g1->GetHoldTime() == g2->GetHoldTime()) {
                    return g1->GetBuyCount() > g2->GetBuyCount();
                }
                return g1->GetHoldTime() > g2->GetHoldTime();
            });
        } else {
            sort(buyers.begin(), buyers.end(),
                 [&](const BGoods* g1, const BGoods* g2) { return g1->GetBuyCount() > g2->GetBuyCount(); });
        }

        buyers_count += buyers.size();
    }
    return ans;
}

void Buyer::assign_goods() {
    auto Seller = Seller::GetInstance();
    auto& global_count = Seller->GetGloalCount();

    for (int intent_id = 0; intent_id < 5; ++intent_id) {
        int buyers_count = 0;
        unordered_map<string, int> ump_count;
        auto buyers_gr = create_buyers(intent_id, buyers_count, ump_count);

        log_debug("第 %d 轮, 意向数: %d, 买家总数: %d", intent_id, buyers_gr.size(), buyers_count);
        int idx = 0;

        if (intent_id == 0) {
            while (true) {
                // [step1] 选 (库存 / 购买数量) 最少的
                int select_idx = -1;
                int select_buy_count = 0;
                double select_value = 0;

                for (int i = 0; i < buyers_gr.size(); ++i) {
                    if (buyers_gr[i].second.empty()) continue;
                    const auto& intent = buyers_gr[i].first;
                    int buy_count = ump_count[intent];
                    int stock = global_count[intent];
                    if (stock == 0 || buy_count == 0) continue;
                    double value = (double)(stock * 1.0) / (double)(buy_count * 1.0);
                    double d = abs(value - select_value);
                    if (select_idx == -1 || value < select_value || (d <= 1e-3 && buy_count > select_buy_count)) {
                        select_idx = i;
                        select_value = value;
                        select_buy_count = buy_count;
                    }
                }
                if (select_idx == -1) break;

                // [step2] 满足约束的时候按照购买数量倒序
                int select_sort_idx = 0, count = 0,
                    tol_count = global_count[buyers_gr[select_idx].second.front()->GetIntentMapKeys()[0]];
                for (auto& buyer : buyers_gr[select_idx].second) {
                    count += buyer->GetBuyCount();
                    if (count > tol_count) break;
                    select_sort_idx++;
                }

                sort(buyers_gr[select_idx].second.begin(), buyers_gr[select_idx].second.begin() + select_sort_idx,
                     [&](const BGoods* g1, const BGoods* g2) { return g1->GetBuyCount() > g2->GetBuyCount(); });

                // [step3] 分配货物
                auto buyer = buyers_gr[select_idx].second.front();
                buyers_gr[select_idx].second.pop_front();
                int x = buyer->GetBuyCount();

                if (++idx % 10000 == 0) log_debug("* idx: %d, 成交数目: %d", idx, m_results.size());
                this->assign_buyer(buyer, intent_id);

                int y = buyer->GetBuyCount();
                ump_count[buyers_gr[select_idx].first] -= (y - x);
            }
            continue;
        }

        vector<BGoods*> buyers;
        for (auto& it : buyers_gr) {
            buyers.insert(buyers.end(), it.second.begin(), it.second.end());
        }
        sort(buyers.begin(), buyers.end(),
             [&](const BGoods* g1, const BGoods* g2) { return g1->GetBuyCount() > g2->GetBuyCount(); });
        for (auto& buyer : buyers) {
            if (++idx % 10000 == 0) log_debug("* idx: %d, 成交数目: %d", idx, m_results.size());
            this->assign_buyer(buyer, intent_id);
        }
    }
    this->assign_last_buyers();
}
