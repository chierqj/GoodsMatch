#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <fstream>
#include <map>
#include <queue>
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
// int factor[6] = {0, 1, 1, 10, 50, 90};  // 本地:157.153 线上: 157.19000244000
int factor[6] = {0, 1, 1, 10, 50, 90};  // 本地:157.153 线上: 157.19000244000

Depot* choice_best_depot_step1(BGoods* buyer) {
    auto Seller = Seller::GetInstance();
    auto& sellers_groupby_depot = Seller->GetSellers();

    int max_value = 0;
    string max_depot_id;

    const auto& first_intent = buyer->GetIntentMapKeys()[0];
    for (auto& [depot_id, depot] : sellers_groupby_depot) {
        int stock = depot->sellers_tol_stock[first_intent];
        if (stock == 0) continue;

        int val = 0;
        for (auto& it : buyer->GetPermuIntents()) {
            if (it->orders[0] != 0) continue;
            val += depot->sellers_tol_stock[it->map_key] * factor[(it->values).size()];
        }
        if (val > max_value) {
            max_value = val;
            max_depot_id = depot_id;
        }
    }

    if (max_value == 0) return nullptr;
    Depot* result = max_value == 0 ? nullptr : sellers_groupby_depot[max_depot_id];
    return result;
}

Depot* choice_best_depot_step2(BGoods* buyer) {
    auto Seller = Seller::GetInstance();
    auto& sellers_groupby_depot = Seller->GetSellers();

    int max_value = 0;
    string best_depot_id;

    for (auto& [depot_id, depot] : sellers_groupby_depot) {
        int val = 0;
        for (auto& intent : buyer->GetPermuIntents()) {
            val += depot->sellers_tol_stock[intent->map_key] * factor[(intent->values).size()];
        }
        if (val > max_value) {
            max_value = val;
            best_depot_id = depot_id;
        }
    }

    Depot* result = max_value == 0 ? nullptr : sellers_groupby_depot[best_depot_id];
    return result;
}

// void Buyer::assign_buyer(BGoods* buyer, bool consider_first_intent) {
//     struct seller_node {
//         Intent* expecter;
//         int stock;
//         bool operator<(const seller_node& r) const { return expecter->score < r.expecter->score; }
//     };

//     auto& global_count = Seller::GetInstance()->GetGlobalCount();

//     while (buyer->GetBuyCount() > 0) {
//         // [step1] 选一个最优的仓库
//         auto depot = (consider_first_intent ? choice_best_depot_step1(buyer) : choice_best_depot_step2(buyer));

//         if (depot == nullptr) break;

//         // [step3] 枚举27种意向，丢到pq里面
//         priority_queue<seller_node> pq_sellers;
//         for (const auto& expecter : buyer->GetPermuIntents()) {
//             // 必须考虑第一意向的情况下
//             if (consider_first_intent && expecter->orders[0] != 0) continue;
//             depot->pop_front(buyer->GetBreed(), expecter->map_key);
//             if (depot->sellers_groupby_except[expecter->map_key].empty()) continue;

//             sort(depot->sellers_groupby_except[expecter->map_key].begin(),
//                  depot->sellers_groupby_except[expecter->map_key].end(),
//                  [&](const SGoods* g1, const SGoods* g2) { return g1->GetGoodStock() > g2->GetGoodStock(); });
//             pq_sellers.push({expecter});
//         }

//         // log_debug("* debug1");
//         while (buyer->GetBuyCount() > 0 && !pq_sellers.empty()) {
//             auto head = pq_sellers.top();
//             pq_sellers.pop();

//             auto& seller = depot->sellers_groupby_except[head.expecter->map_key].front();

//             int val = min(buyer->GetBuyCount(), seller->GetGoodStock());

//             buyer->SetBuyCount(buyer->GetBuyCount() - val);
//             seller->SetGoodStock(seller->GetGoodStock() - val);
//             depot->decrease_stock(seller, val, global_count);

//             Business bus(buyer, seller, val, head.expecter->orders);
//             m_results.push_back(bus);

//             depot->pop_front(buyer->GetBreed(), head.expecter->map_key);
//             if (depot->sellers_groupby_except[head.expecter->map_key].empty()) continue;

//             pq_sellers.push(head);
//         }
//     }
// }

void Buyer::assign_buyer(BGoods* buyer, bool consider_first_intent) {
    auto& global_count = Seller::GetInstance()->GetGlobalCount();

    while (buyer->GetBuyCount() > 0) {
        // [step1] 选一个最优的仓库
        auto depot = (consider_first_intent ? choice_best_depot_step1(buyer) : choice_best_depot_step2(buyer));

        if (depot == nullptr) break;

        auto& permu_intenters = buyer->GetPermuIntents();
        sort(permu_intenters.begin(), permu_intenters.end(), [&](const Intent* g1, const Intent* g2) {
            int count1 = depot->sellers_tol_stock[g1->map_key], count2 = depot->sellers_tol_stock[g2->map_key];
            if (g1->score == g2->score) {
                return count1 > count2;
            }
            return g1->score > g2->score;
        });

        // [step2] 得分从大到小枚举意向组合
        for (auto& intenter : permu_intenters) {
            if (buyer->GetBuyCount() <= 0) break;

            // 必须考虑第一意向的情况下
            if (consider_first_intent && intenter->orders[0] != 0) continue;

            depot->pop_front(buyer->GetBreed(), intenter->map_key);
            auto& sellers = depot->sellers_groupby_except[intenter->map_key];
            if (sellers.empty()) continue;

            sort(sellers.begin(), sellers.end(),
                 [&](const SGoods* g1, const SGoods* g2) { return g1->GetGoodStock() > g2->GetGoodStock(); });

            while (buyer->GetBuyCount() > 0 && !sellers.empty()) {
                auto& seller = sellers.front();

                int val = min(buyer->GetBuyCount(), seller->GetGoodStock());

                buyer->SetBuyCount(buyer->GetBuyCount() - val);
                seller->SetGoodStock(seller->GetGoodStock() - val);
                depot->decrease_stock(seller, val, global_count);

                Business bus(buyer, seller, val, intenter->orders);
                m_results.push_back(bus);

                depot->pop_front(buyer->GetBreed(), intenter->map_key);
            }
        }
    }
}

void Buyer::assign_last_buyers() {
    for (auto& buyer : m_goods) {
        if (buyer->GetBuyCount() > 0) {
            m_buyers_no_expects.push_back(buyer);
        }
    }
    sort(m_buyers_no_expects.begin(), m_buyers_no_expects.end(),
         [&](const BGoods* g1, const BGoods* g2) { return g1->GetBuyCount() > g2->GetBuyCount(); });

    log_debug("第 last 轮, 买家总数: %d", m_buyers_no_expects.size());

    auto Seller = Seller::GetInstance();
    auto& sellers_groupby_depot = Seller->GetSellers();
    auto& global_count = Seller->GetGlobalCount();

    for (auto& buyer : m_buyers_no_expects) {
        for (auto& [depot_id, depot] : sellers_groupby_depot) {
            if (buyer->GetBuyCount() <= 0) break;

            while (buyer->GetBuyCount() > 0 && !depot->useful_sellers[buyer->GetBreed()].empty()) {
                auto& seller = *depot->useful_sellers[buyer->GetBreed()].begin();
                int val = min(buyer->GetBuyCount(), seller->GetGoodStock());

                buyer->SetBuyCount(buyer->GetBuyCount() - val);
                seller->SetGoodStock(seller->GetGoodStock() - val);
                depot->decrease_stock(seller, val, global_count);

                Business bus(buyer, seller, val, {0});
                m_results.push_back(bus);

                if (seller->GetGoodStock() <= 0) {
                    depot->useful_sellers[buyer->GetBreed()].erase(seller);
                }
            }
        }
    }
}

vector<vector<BGoods*>> Buyer::create_buyers(int intent_id, int& buyers_count) {
    unordered_map<string, vector<BGoods*>> ump;
    unordered_map<string, int> ump_count;
    auto& global_count = Seller::GetInstance()->GetGlobalCount();
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

    vector<vector<BGoods*>> ans;
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
        ans.push_back(buyers);
    }

    return ans;
}

void Buyer::assign_goods() {
    for (int i = 0; i < 5; ++i) {
        int buyers_count = 0;
        auto vct_buyers = create_buyers(i, buyers_count);
        log_debug("第 %d 轮, 意向数: %d, 买家总数: %d", i, vct_buyers.size(), buyers_count);
        int idx = 0;
        for (auto& buyers : vct_buyers) {
            for (auto& buyer : buyers) {
                if (++idx % 10000 == 0) log_debug("* idx: %d", idx);
                this->assign_buyer(buyer, (i == 0));
            }
        }
    }
    this->assign_last_buyers();
}
