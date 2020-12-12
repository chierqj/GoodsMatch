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

// int factor[6] = {1, 2, 2, 20, 40};  // 157.00999451000
// int factor[6] = {0, 1, 2, 10, 20, 40};  // 157.048
// int factor[6] = {0, 1, 2, 10, 30, 40};  // 157.14999390
// int factor[6] = {0, 1, 1, 10, 50, 90};  // 本地:157.153 线上: 157.19000244
// int factor[6] = {0, 1, 1, 10, 300, 1000};  // 本地:157.173 线上: 157.21000671
int factor[6] = {0, 1, 1, 10, 300, 1000};  // 本地:157.173 线上: 157.19000244000
int cf_score[5] = {33, 27, 20, 13, 7};
int sr_score[5] = {40, 30, 20, 10, 0};

int Buyer::get_depot_score(Depot* depot, BGoods* buyer, bool consider_first_intent) {
    auto& items = depot->sellers[buyer->GetBreed()];
    depot->pop_front(buyer);

    bool ok = false;
    for (auto& item : items) {
        item.score = 0;
        item.pop_front();
        if (item.values.empty()) continue;

        // 意向打分
        auto intent_order = this->get_intent_order(item.values.front(), buyer);
        if (consider_first_intent && (intent_order.empty() || intent_order[0] != 0)) continue;

        ok = true;
        int intent_score = 0;
        for (auto& it : intent_order) {
            intent_score += (buyer->GetBreed() == "CF" ? cf_score[it] : sr_score[it]);
        }
        item.score = intent_score * factor[intent_order.size()];
    }

    if (!ok) return 0;

    sort(items.begin(), items.end(), [&](const auto& it1, const auto& it2) {
        if (it1.score == it2.score) {
            return it1.tol_stock > it2.tol_stock;
        }
        return it1.score > it2.score;
    });
    int result = 0, tol = 0;
    for (auto& item : items) {
        if (tol >= buyer->GetBuyCount()) break;
        int count = min(buyer->GetBuyCount() - tol, item.tol_stock);
        result += count * item.score;
        tol += count;
    }
    return result;
}

void Buyer::do_business(Depot::Item& item, BGoods* buyer, SGoods* seller) {
    auto Seller = Seller::GetInstance();
    auto& global_count = Seller->GetGloalCount();

    int val = min(buyer->GetBuyCount(), seller->GetGoodStock());
    buyer->SetBuyCount(buyer->GetBuyCount() - val);
    seller->SetGoodStock(seller->GetGoodStock() - val);
    item.tol_stock -= val;

    for (auto& it : Seller->GetPropotys()[seller->GetGoodID()]) {
        global_count[it->map_key] -= val;
    }

    Business bus(buyer, seller, val, this->get_intent_order(seller, buyer));
    m_results.push_back(bus);
}

void Buyer::assign_buyer(BGoods* buyer, bool consider_first_intent) {
    auto Seller = Seller::GetInstance();
    auto& global_count = Seller->GetGloalCount();
    if (consider_first_intent && global_count[buyer->GetIntentMapKeys()[0]] <= 0) return;

    while (buyer->GetBuyCount() > 0) {
        int select_score = 0;
        Depot* select_depot = nullptr;

        // [step1] 选一个仓库
        for (auto& depot : Seller->GetDepots()) {
            int score = get_depot_score(depot, buyer, consider_first_intent);
            if (score > select_score) {
                select_score = score;
                select_depot = depot;
            }
        }
        if (select_depot == nullptr) {
            break;
        }

        // log_debug("----------- 选择仓库买之前 -----------");

        // [step2] 按照品种选sellers. groupby_goodid
        select_depot->pop_front(buyer);
        auto& items = select_depot->sellers[buyer->GetBreed()];

        // [step3] 货物编号的score降序
        while (buyer->GetBuyCount() > 0 && !items.empty()) {
            auto& item = items.front();
            item.pop_front();

            // [step4] 当前货物编号挨个选
            while (buyer->GetBuyCount() > 0 && !item.values.empty()) {
                auto& seller = item.values.front();
                this->do_business(item, buyer, seller);
                item.pop_front();
            }

            select_depot->pop_front(buyer);
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
        if (++idx % 1 == 0) log_debug("* idx: %d", idx);
        for (auto& depot : Seller->GetDepots()) {
            depot->pop_front(buyer);
            auto& items = depot->sellers[buyer->GetBreed()];

            while (buyer->GetBuyCount() > 0 && !items.empty()) {
                auto& item = items.front();
                item.pop_front();
                while (buyer->GetBuyCount() > 0 && !item.values.empty()) {
                    auto& seller = item.values.front();
                    this->do_business(item, buyer, seller);
                    item.pop_front();
                }

                depot->pop_front(buyer);
            }
        }
    }
}

vector<queue<BGoods*>> Buyer::create_buyers(int intent_id, int& buyers_count) {
    unordered_map<string, vector<BGoods*>> ump;
    unordered_map<string, int> ump_count;

    for (const auto& buyer : m_goods) {
        if (buyer->GetBuyCount() <= 0 || buyer->GetIntentMapKeys()[intent_id].empty()) {
            continue;
        }
        const string& key = buyer->GetIntentMapKeys()[intent_id];
        ump[key].push_back(buyer);
        ump_count[key] += buyer->GetBuyCount();
    }
    vector<pair<int, string>> vt;
    for (auto& [intent, buyers] : ump) {
        vt.push_back({ump_count[intent], intent});
    }
    sort(vt.begin(), vt.end(),
         [&](const pair<int, string>& p1, const pair<int, string>& p2) { return p1.first > p2.first; });

    vector<queue<BGoods*>> ans;
    buyers_count = 0;
    for (auto& it : vt) {
        auto buyers = ump[it.second];
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
        queue<BGoods*> tmp;
        for (auto& it : buyers) tmp.push(it);
        ans.push_back(tmp);
    }

    return ans;
}

void Buyer::assign_goods() {
    auto Seller = Seller::GetInstance();
    auto& global_count = Seller->GetGloalCount();

    for (int intent_id = 0; intent_id < 5; ++intent_id) {
        int buyers_count = 0;
        auto buyers_gr = create_buyers(intent_id, buyers_count);

        log_debug("第 %d 轮, 意向数: %d, 买家总数: %d", intent_id, buyers_gr.size(), buyers_count);
        int idx = 0;

        // for (auto& buyers : buyers_gr) {
        //     while (!buyers.empty()) {
        //         if (++idx % 1000 == 0) log_debug("* idx: %d", idx);
        //         auto& buyer = buyers.front();
        //         buyers.pop();
        //         this->assign_buyer(buyer, (intent_id == 0));
        //     }
        // }

        while (true) {
            if (++idx % 1000 == 0) log_debug("* idx: %d", idx);

            int select_idx = -1;
            double select_value = INT_MAX;
            for (int i = 0; i < buyers_gr.size(); ++i) {
                const auto& buyers = buyers_gr[i];
                if (buyers.empty()) continue;
                const auto& buyer = buyers.front();
                double val = (double)(global_count[buyer->GetIntentMapKeys()[intent_id]] * 1.0);
                val /= (double)(buyer->GetBuyCount() * 1.0);
                if (select_idx == -1 || val < select_value) {
                    select_idx = i;
                    select_value = val;
                }
            }
            if (select_idx == -1) break;

            auto& buyer = buyers_gr[select_idx].front();
            buyers_gr[select_idx].pop();

            this->assign_buyer(buyer, (intent_id == 0));
        }
    }
    this->assign_last_buyers();
}
