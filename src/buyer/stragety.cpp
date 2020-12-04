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

int res_count1 = 0;
int res_count2 = 0;

int choice_best_depot(BGoods* buyer, const string& first_expect, string& best_depot_id) {
    auto Seller = Seller::GetInstance();
    auto& sellers_groupby_depot = Seller->GetSellers();

    int max_value = 0;
    int max_sum_value = 0;
    string s1, s2;

    for (auto& [depot_id, depot] : sellers_groupby_depot) {
        int stock = depot->sellers_tol_stock[first_expect];
        if (stock == 0) continue;
        if (stock > max_value) {
            max_value = stock;
            s1 = depot_id;
        }

        int tmp = 0;
        for (auto& it : buyer->GetPermuIntents()) {
            if (it->orders[0] != 0) continue;
            string key = it->map_key;
            int cnt = depot->sellers_tol_stock[key];
            tmp += cnt;
        }
        if (tmp > max_sum_value) {
            max_sum_value = tmp;
            s2 = depot_id;
        }
    }

    if (max_value == 0) return -1;
    if (max_value < buyer->GetBuyCount()) {
        ++res_count1;
        best_depot_id = s1;
    } else {
        ++res_count2;
        best_depot_id = s2;
    }

    return 0;
}

void Buyer::do_assign_step1(vector<BGoods*>& left_buyers) {
    struct big_buyer_node {
        string first_expect;
        int buyer_idx;
        int buy_count;
        bool operator<(const big_buyer_node& r) const {
            auto Seller = Seller::GetInstance();
            auto& sellers_groupby_depot = Seller->GetSellers();
            auto& global_count = Seller->GetGlobalCount();
            double s1 = (double)buy_count / (double)global_count[first_expect];
            double s2 = (double)r.buy_count / (double)global_count[r.first_expect];
            return s1 > s2;
        }
    };

    auto Seller = Seller::GetInstance();
    auto& sellers_groupby_depot = Seller->GetSellers();
    auto& global_count = Seller->GetGlobalCount();

    // 每组buyers group拿出第一个buyer
    priority_queue<big_buyer_node> pq_buyers;
    for (const auto& [expect, buyers] : m_buyers_groupby_expect) {
        pq_buyers.push({expect, 0, buyers.front()->GetBuyCount()});
    }

    int idx = 0;

    while (!pq_buyers.empty()) {
        if (++idx % 10000 == 0) log_debug("* idx: %d", idx);

        auto big_buyer_node = pq_buyers.top();
        pq_buyers.pop();

        auto& buyer = m_buyers_groupby_expect[big_buyer_node.first_expect][big_buyer_node.buyer_idx];

        while (buyer->GetBuyCount() > 0) {
            // [step1] 选一个最优的仓库
            string best_depot_id;
            int iRet = choice_best_depot(buyer, big_buyer_node.first_expect, best_depot_id);
            if (iRet != 0) break;

            auto& depot = sellers_groupby_depot[best_depot_id];

            struct seller_node {
                Intent* expecter;
                int stock;
                bool operator<(const seller_node& r) const {
                    return expecter->score * stock < r.expecter->score * r.stock;
                }
            };

            // [step3] 枚举27种意向，丢到pq里面
            priority_queue<seller_node> pq_sellers;
            for (const auto& expecter : buyer->GetPermuIntents()) {
                if (expecter->orders[0] != 0) continue;
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
                // depot->decrease_stock(seller, val);
                depot->decrease_stock(seller, val, global_count);

                Business bus(buyer, seller, val, seller_node_head.expecter->orders);
                m_results.push_back(bus);

                depot->pop_front(buyer->GetBreed(), seller_node_head.expecter->map_key);
                if (depot->sellers_groupby_except[seller_node_head.expecter->map_key].empty()) continue;

                pq_sellers.push(seller_node_head);
            }
        }

        if (buyer->GetBuyCount() > 0) {
            left_buyers.push_back(buyer);
        }

        if (big_buyer_node.buyer_idx + 1 < m_buyers_groupby_expect[big_buyer_node.first_expect].size()) {
            big_buyer_node.buyer_idx++;
            big_buyer_node.buy_count =
                m_buyers_groupby_expect[big_buyer_node.first_expect][big_buyer_node.buyer_idx]->GetBuyCount();
            pq_buyers.push(big_buyer_node);
        }
    }

    log_info("---------------------[step1]------------------------");
    log_info("* left_buyers: %d", left_buyers.size());
    for (auto& [depot_id, depot] : sellers_groupby_depot) {
        for (auto& [breed, sellers] : depot->useful_sellers) {
            if (sellers.size() == 0) continue;
            log_info("* %s|%s: %d", depot_id.c_str(), breed.c_str(), sellers.size());
        }
    }
    log_info("----------------------------------------------------");
}

void Buyer::do_assign_step2(vector<BGoods*>& left_buyers) {
    vector<BGoods*> tmp_buyers = left_buyers;
    left_buyers.clear();
    sort(tmp_buyers.begin(), tmp_buyers.end(),
         [&](const BGoods* g1, const BGoods* g2) { return g1->GetBuyCount() > g2->GetBuyCount(); });

    auto Seller = Seller::GetInstance();
    auto& sellers_groupby_depot = Seller->GetSellers();

    for (auto& buyer : tmp_buyers) {
        while (buyer->GetBuyCount() > 0) {
            string best_depot_id;
            int iRet = choice_best_depot(buyer, best_depot_id);
            if (iRet != 0) break;

            auto& depot = sellers_groupby_depot[best_depot_id];

            for (const auto& intent : buyer->GetPermuIntents()) {
                if (buyer->GetBuyCount() <= 0) break;
                while (buyer->GetBuyCount() > 0) {
                    depot->pop_front(buyer->GetBreed(), intent->map_key);
                    if (depot->sellers_groupby_except[intent->map_key].empty()) break;

                    auto& seller = depot->sellers_groupby_except[intent->map_key].front();
                    int val = min(buyer->GetBuyCount(), seller->GetGoodStock());

                    buyer->SetBuyCount(buyer->GetBuyCount() - val);
                    seller->SetGoodStock(seller->GetGoodStock() - val);
                    depot->decrease_stock(seller, val);

                    Business bus(buyer, seller, val, intent->orders);
                    m_results.push_back(bus);
                }
            }
        }

        if (buyer->GetBuyCount() > 0) {
            left_buyers.push_back(buyer);
        }
    }

    log_info("---------------------[step2]------------------------");
    log_info("* left_buyers: %d", left_buyers.size());
    for (auto& [depot_id, depot] : sellers_groupby_depot) {
        for (auto& [breed, sellers] : depot->useful_sellers) {
            if (sellers.size() == 0) continue;
            log_info("* %s|%s: %d", depot_id.c_str(), breed.c_str(), sellers.size());
        }
    }
    log_info("----------------------------------------------------");
}

void Buyer::do_assign_step3(vector<BGoods*>& left_buyers) {
    left_buyers.insert(left_buyers.end(), m_buyers_no_expects.begin(), m_buyers_no_expects.end());
    sort(left_buyers.begin(), left_buyers.end(),
         [&](const BGoods* g1, const BGoods* g2) { return g1->GetBuyCount() > g2->GetBuyCount(); });

    auto Seller = Seller::GetInstance();
    auto& sellers_groupby_depot = Seller->GetSellers();

    for (auto& buyer : left_buyers) {
        for (auto& [depot_id, depot] : sellers_groupby_depot) {
            if (buyer->GetBuyCount() <= 0) break;

            while (buyer->GetBuyCount() > 0 && !depot->useful_sellers[buyer->GetBreed()].empty()) {
                auto& seller = *depot->useful_sellers[buyer->GetBreed()].begin();
                int val = min(buyer->GetBuyCount(), seller->GetGoodStock());

                buyer->SetBuyCount(buyer->GetBuyCount() - val);
                seller->SetGoodStock(seller->GetGoodStock() - val);
                depot->decrease_stock(seller, val);

                Business bus(buyer, seller, val, {0});
                m_results.push_back(bus);

                if (seller->GetGoodStock() <= 0) {
                    depot->useful_sellers[buyer->GetBreed()].erase(seller);
                }
            }
        }
    }

    log_info("---------------------[step3]------------------------");
    log_info("* left_buyers: %d", left_buyers.size());
    for (auto& [depot_id, depot] : sellers_groupby_depot) {
        for (auto& [breed, sellers] : depot->useful_sellers) {
            if (sellers.size() == 0) continue;
            log_info("* %s|%s: %d", depot_id.c_str(), breed.c_str(), sellers.size());
        }
    }
    log_info("----------------------------------------------------");
}

void Buyer::assign_goods() {
    vector<BGoods*> left_buyers;
    this->do_assign_step1(left_buyers);
    this->do_assign_step2(left_buyers);
    this->do_assign_step3(left_buyers);

    log_debug("* count1: %d", res_count1);
    log_debug("* count2: %d", res_count2);
}
