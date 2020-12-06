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

// int factor[6] = {1, 2, 5, 15, 30};
int factor[6] = {1, 2, 2, 20, 40};  // 157.00999451000

Depot* choice_best_depot_step1(BGoods* buyer) {
    auto Seller = Seller::GetInstance();
    auto& sellers_groupby_depot = Seller->GetSellers();

    int max_value = 0;
    string max_depot_id;

    for (auto& [depot_id, depot] : sellers_groupby_depot) {
        if (depot->sellers_tol_stock[buyer->GetFirstIntent()] == 0) continue;

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

Depot* choice_best_depot_step2(const BGoods* buyer) {
    auto Seller = Seller::GetInstance();
    auto& sellers_groupby_depot = Seller->GetSellers();

    int max_value = 0;
    string best_depot_id;

    for (auto& [depot_id, depot] : sellers_groupby_depot) {
        int val = 0;
        for (auto& intent : buyer->GetPermuIntents()) {
            if (intent->orders[0] == 0) continue;
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

BGoods* Buyer::get_best_buyer() {
    auto Seller = Seller::GetInstance();
    auto& sellers_groupby_depot = Seller->GetSellers();
    auto& global_count = Seller->GetGlobalCount();

    bool fir = true;
    double min_value = 0;
    string min_intent;
    bool ok = false;

    for (auto& [intent, buyers] : m_buyers_groupby_expect) {
        if (buyers.empty()) continue;

        ok = true;
        const auto& buyer = buyers.front();
        double val = (double)(global_count[intent] * 1.0) / (double)(buyer->GetBuyCount() * 1.0);

        if (fir == true || val < min_value) {
            fir = false;
            min_value = val;
            min_intent = intent;
        }
    }
    if (!ok) return nullptr;

    auto result = m_buyers_groupby_expect[min_intent].front();
    m_buyers_groupby_expect[min_intent].pop_front();
    return result;
}

void Buyer::assign_buyer(BGoods* buyer, bool consider_first_intent) {
    struct seller_node {
        Intent* expecter;
        int stock;
        bool operator<(const seller_node& r) const { return expecter->score < r.expecter->score; }
    };

    auto& global_count = Seller::GetInstance()->GetGlobalCount();

    while (buyer->GetBuyCount() > 0) {
        // [step1] 选一个最优的仓库
        auto depot = (consider_first_intent ? choice_best_depot_step1(buyer) : choice_best_depot_step2(buyer));

        if (depot == nullptr) break;

        // [step3] 枚举27种意向，丢到pq里面
        priority_queue<seller_node> pq_sellers;
        for (const auto& expecter : buyer->GetPermuIntents()) {
            // 必须考虑第一意向的情况下
            if (consider_first_intent && expecter->orders[0] != 0) continue;
            depot->pop_front(buyer->GetBreed(), expecter->map_key);
            if (depot->sellers_groupby_except[expecter->map_key].empty()) continue;
            pq_sellers.push({expecter});
        }

        // log_debug("* debug1");
        while (buyer->GetBuyCount() > 0 && !pq_sellers.empty()) {
            auto seller_node_head = pq_sellers.top();
            pq_sellers.pop();

            auto& seller = depot->sellers_groupby_except[seller_node_head.expecter->map_key].front();

            int val = min(buyer->GetBuyCount(), seller->GetGoodStock());

            buyer->SetBuyCount(buyer->GetBuyCount() - val);
            seller->SetGoodStock(seller->GetGoodStock() - val);
            depot->decrease_stock(seller, val, global_count);

            Business bus(buyer, seller, val, seller_node_head.expecter->orders);
            m_results.push_back(bus);

            depot->pop_front(buyer->GetBreed(), seller_node_head.expecter->map_key);
            if (depot->sellers_groupby_except[seller_node_head.expecter->map_key].empty()) continue;

            pq_sellers.push(seller_node_head);
        }
    }
}

void Buyer::assign_step1() {
    // int idx = 0;
    // while (true) {
    //     if (++idx % 10000 == 0) log_debug("* idx: %d", idx);

    //     auto buyer = this->get_best_buyer();
    //     if (buyer == nullptr) break;

    //     this->assign_buyer(buyer, true);

    //     if (buyer->GetBuyCount() > 0) {
    //         m_left_buyers.push_back(buyer);
    //     }
    // }

    int idx = 0;
    for (auto& intent : m_range_order) {
        for (auto& buyer : m_buyers_groupby_expect[intent]) {
            if (++idx % 10000 == 0) log_debug("* idx: %d", idx);

            if (buyer == nullptr) break;

            this->assign_buyer(buyer, true);

            if (buyer->GetBuyCount() > 0) {
                m_left_buyers.push_back(buyer);
            }
        }
    }
}

void Buyer::assign_step2() {
    auto Seller = Seller::GetInstance();
    auto& sellers_groupby_depot = Seller->GetSellers();
    auto& global_count = Seller->GetGlobalCount();

    auto tmp_buyers = m_left_buyers;
    m_left_buyers.clear();

    auto m_sort = [&]() {
        sort(tmp_buyers.begin(), tmp_buyers.end(),
             [&](const BGoods* g1, const BGoods* g2) { return g1->GetBuyCount() > g2->GetBuyCount(); });
    };

    m_sort();
    int idx = 0;

    while (!tmp_buyers.empty()) {
        if (++idx % 10000 == 0) log_debug("* idx: %d", idx);

        auto& buyer = tmp_buyers.front();
        tmp_buyers.pop_front();

        this->assign_buyer(buyer, false);
        if (buyer->GetBuyCount() > 0) {
            m_left_buyers.push_back(buyer);
        }
    }
}

void Buyer::assign_step3() {
    m_left_buyers.insert(m_left_buyers.end(), m_buyers_no_expects.begin(), m_buyers_no_expects.end());
    sort(m_left_buyers.begin(), m_left_buyers.end(),
         [&](const BGoods* g1, const BGoods* g2) { return g1->GetBuyCount() > g2->GetBuyCount(); });

    auto Seller = Seller::GetInstance();
    auto& sellers_groupby_depot = Seller->GetSellers();
    auto& global_count = Seller->GetGlobalCount();

    auto tmp_buyers = m_left_buyers;
    m_left_buyers.clear();

    for (auto& buyer : tmp_buyers) {
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
        if (buyer->GetBuyCount() > 0) {
            m_left_buyers.push_back(buyer);
        }
    }
}

void Buyer::debug_depot(string msg) {
    log_info("---------------------[%s]------------------------", msg.c_str());

    auto Seller = Seller::GetInstance();
    auto& sellers_groupby_depot = Seller->GetSellers();
    auto& global_count = Seller->GetGlobalCount();

    log_info("* left_buyers: %d", m_left_buyers.size());
    for (auto& [depot_id, depot] : sellers_groupby_depot) {
        for (auto& [breed, sellers] : depot->useful_sellers) {
            if (sellers.size() == 0) continue;
            log_info("* %s|%s: %d", depot_id.c_str(), breed.c_str(), sellers.size());
        }
    }
    log_info("----------------------------------------------------");
}

void Buyer::assign_goods() {
    this->assign_step1();
    this->debug_depot("step1");
    this->assign_step2();
    this->debug_depot("step2");
    this->assign_step3();
    this->debug_depot("step3");
}
