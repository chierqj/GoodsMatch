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

void Buyer::do_business(GoodItem* item, BGoods* buyer, SGoods* seller, int intent_id) {
    auto Seller = Seller::GetInstance();
    auto& global_count = Seller->GetGloalCount();

    int val = min(buyer->GetBuyCount(), seller->GetGoodStock());

    buyer->SetBuyCount(buyer->GetBuyCount() - val);
    seller->SetGoodStock(seller->GetGoodStock() - val);
    item->tol_stock -= val;

    for (auto& it : Seller->GetPropotys()[seller->GetGoodID()]) {
        global_count[it->map_key] -= val;
    }

    Business bus(buyer, seller, val, {});
    m_results.push_back(bus);
}

void Buyer::assign_buyer(BGoods* buyer, int intent_id) {
    auto Seller = Seller::GetInstance();
    auto& global_count = Seller->GetGloalCount();

    unordered_set<string> vis_depot;
    int depot_score = 29;

    while (buyer->GetBuyCount() > 0) {
        int max_score = 0;
        int max_stock = 0;
        GoodItem* select_item = nullptr;
        int need = buyer->GetBuyCount();

        int L = intent_id == 0 ? 0 : 1;
        int R = intent_id == 0 ? 1 : 5;

        for (int i = L; i < R; ++i) {
            auto& items = Seller->GetGoodItems()[buyer->GetIntentMapKeys()[i]];
            for (auto& item : items) {
                item->pop_front();
                if (item->empty()) continue;
                if (buyer->GetBuyCount() <= 0) break;
                const auto& seller = item->values.front();
                int score = this->get_intent_score(seller, buyer);
                if (vis_depot.find(seller->GetDepotID()) != vis_depot.end()) score += depot_score;
                if (score > max_score) {
                    max_score = score;
                    select_item = item;
                    max_stock = item->tol_stock;
                } else if (score == max_score && item->tol_stock > max_stock) {
                    select_item = item;
                    max_stock = item->tol_stock;
                }
            }
        }

        if (select_item == nullptr) break;

        sort(select_item->values.begin(), select_item->values.end(),
             [&](const SGoods* g1, const SGoods* g2) { return g1->GetGoodStock() > g2->GetGoodStock(); });

        vis_depot.insert(select_item->values.front()->GetDepotID());
        while (!select_item->empty() && buyer->GetBuyCount() > 0) {
            auto& seller = select_item->values.front();
            this->do_business(select_item, buyer, seller, intent_id);
            select_item->pop_front();
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

    unordered_map<string, vector<GoodItem*>> map_sellers;
    for (auto& [intent, items] : Seller->GetGoodItems()) {
        this->pop_deque(items);
        if (items.empty()) continue;
        for (auto& item : items) {
            item->pop_front();
            if (item->empty()) continue;
            map_sellers[item->values.front()->GetBreed()].push_back(item);
        }
    }

    int idx = 0;
    for (auto& buyer : last_buyers) {
        if (++idx % 1000 == 0) log_debug("* idx: %d", idx);
        for (auto& item : map_sellers[buyer->GetBreed()]) {
            if (buyer->GetBuyCount() <= 0) break;

            item->pop_front();
            if (item->empty()) continue;
            if (item->values.front()->GetBreed() != buyer->GetBreed()) continue;

            while (!item->empty() && buyer->GetBuyCount() > 0) {
                auto& seller = item->values.front();
                this->do_business(item, buyer, seller, 0);
                item->pop_front();
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

    auto& global_count = Seller::GetInstance()->GetGloalCount();
    sort(ans.begin(), ans.end(), [&](const pair<string, deque<BGoods*>>& p1, pair<string, deque<BGoods*>>& p2) {
        double value1 = (double)(global_count[p1.first] * 1.0) / (double)(ump_count[p1.first] * 1.0);
        double value2 = (double)(global_count[p2.first] * 1.0) / (double)(ump_count[p2.first] * 1.0);
        if (value1 >= 1 && value2 >= 1) {
            return ump_count[p1.first] > ump_count[p2.first];
        }
        return value1 < value2;
    });
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
            for (auto& [intent, buyers] : buyers_gr) {
                while (!buyers.empty()) {
                    if (idx % 100 == 0) {
                        int select_sort_idx = 0, count = 0,
                            tol_count = global_count[buyers.front()->GetIntentMapKeys()[0]];
                        for (auto& buyer : buyers) {
                            count += buyer->GetBuyCount();
                            if (count > tol_count) break;
                            select_sort_idx++;
                        }
                        sort(buyers.begin(), buyers.begin() + select_sort_idx, [&](BGoods* g1, BGoods* g2) {
                            int score1 = 0, count1 = 0, cnt1 = 2;
                            int score2 = 0, count2 = 0, cnt2 = 2;

                            for (auto& it : g1->GetPermuIntents()) {
                                if (it->orders[0] != 0) continue;
                                if (cnt1 <= 0) break;
                                if (global_count[it->map_key] > 0) {
                                    score1 += it->score;
                                    count1 += global_count[it->map_key];
                                    --cnt1;
                                }
                            }
                            for (auto& it : g2->GetPermuIntents()) {
                                if (it->orders[0] != 0) continue;
                                if (cnt2 <= 0) break;
                                if (global_count[it->map_key] > 0) {
                                    score2 += it->score;
                                    count2 += global_count[it->map_key];
                                    --cnt2;
                                }
                            }

                            if (score1 == score2) {
                                double v1 = (double)(count1 * 1.0) / (double)(g1->GetBuyCount() * 1.0);
                                double v2 = (double)(count2 * 1.0) / (double)(g2->GetBuyCount() * 1.0);
                                return v1 < v2;
                            }
                            return score1 > score2;
                        });
                    }

                    auto buyer = buyers.front();
                    if (++idx % 10000 == 0) {
                        log_debug("* idx: %d, 成交数目: %d", idx, m_results.size());
                    }
                    this->assign_buyer(buyer, intent_id);
                    buyers.pop_front();
                }
            }
            continue;
        }

        vector<BGoods*> buyers;
        for (auto& it : buyers_gr) {
            buyers.insert(buyers.end(), it.second.begin(), it.second.end());
        }
        sort(buyers.begin(), buyers.end(),
             [&](BGoods* g1, BGoods* g2) { return g1->GetBuyCount() > g2->GetBuyCount(); });
        for (auto& buyer : buyers) {
            if (++idx % 10000 == 0) {
                log_debug("* idx: %d, 成交数目: %d", idx, m_results.size());
            }
            this->assign_buyer(buyer, intent_id);
        }
    }
    this->assign_last_buyers();
}
