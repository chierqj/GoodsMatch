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

const int cf_score[5] = {33, 27, 20, 13, 7};
const int sr_score[5] = {40, 30, 20, 10, 0};

int get_score(const BGoods* buyer, const SGoods* seller) {
    unordered_map<string, string> ump;
    ump["仓库"] = seller->GetDepotID();
    ump["品牌"] = seller->GetBrand();
    ump["产地"] = seller->GetPlace();
    ump["年度"] = seller->GetYear();
    ump["等级"] = seller->GetLevel();
    ump["类别"] = seller->GetCategory();
    int score = 0;
    for (int i = 0; i < buyer->GetExcepts().size(); ++i) {
        const auto& expect = buyer->GetExcepts()[i];
        if (ump[expect.first] == expect.second) {
            score += (buyer->GetBreed() == "SR" ? sr_score[i] : 0);
            score += (buyer->GetBreed() == "CF" ? cf_score[i] : 0);
        }
    }
    // score *= min(seller->GetGoodStock(), buyer->GetBuyCount());
    return score;
}

void Buyer::do_assign_step1(vector<BGoods*>& left_buyers) {
    left_buyers.clear();
    auto Seller = Seller::GetInstance();
    auto tol_left_sellers = Seller->GetLeftGoods();

    int idx = 0;
    struct pq_ptr {
        string first_expect;
        int idx;
        int val;
        bool operator<(const pq_ptr& r) const { return val < r.val; }
    };
    priority_queue<pq_ptr> Q;

    for (auto& [k, buyers] : m_blocks) {
        Q.push({k, 0, buyers.front()->GetBuyCount()});
    }

    while (!Q.empty()) {
        if (++idx % 1000 == 0) log_debug("* idx: %d", idx++);

        auto pq_head = Q.top();
        Q.pop();

        auto& buyer = m_blocks[pq_head.first_expect][pq_head.idx];

        auto& left_sellers = tol_left_sellers[buyer->GetBreed()];
        vector<vector<pair<string, string>>> permu_expects;
        vector<vector<int>> permu_expect_orders;

        buyer->GetPermutation(permu_expects, permu_expect_orders);

        struct pq_node {
            int score;
            HashMapItr seller_itr;
            int permu_idx;
            bool operator<(const pq_node& r) const { return score > r.score; }
        };
        priority_queue<pq_node> pq;

        for (int i = 0; i < permu_expects.size(); ++i) {
            if (permu_expect_orders[i][0] != 0) continue;
            sort(permu_expects[i].begin(), permu_expects[i].end());
            HashMapItr sellers_itr;
            int iRet = Seller->QueryGoods(sellers_itr, permu_expects[i], buyer->GetBreed());
            if (iRet != 0) continue;  // 找不到
            while (!sellers_itr->second.empty() && sellers_itr->second.front()->GetGoodStock() <= 0) {
                left_sellers.erase(sellers_itr->second.front());
                sellers_itr->second.pop_front();
            }
            if (sellers_itr->second.empty()) continue;
            int score = get_score(buyer, sellers_itr->second.front());
            pq.push(pq_node{score, sellers_itr, i});
        }

        while (buyer->GetBuyCount() > 0 && !pq.empty()) {
            auto head = pq.top();
            pq.pop();

            const auto seller = head.seller_itr->second.front();
            int val = min(buyer->GetBuyCount(), seller->GetGoodStock());

            buyer->SetBuyCount(buyer->GetBuyCount() - val);
            seller->SetGoodStock(seller->GetGoodStock() - val);

            Business bus(buyer, seller, val, permu_expect_orders[head.permu_idx]);
            m_results.push_back(bus);

            while (!head.seller_itr->second.empty() && head.seller_itr->second.front()->GetGoodStock() <= 0) {
                left_sellers.erase(head.seller_itr->second.front());
                head.seller_itr->second.pop_front();
            }

            if (!head.seller_itr->second.empty()) {
                int score = get_score(buyer, head.seller_itr->second.front());
                head.score = score;
                pq.push(head);
            }
        }

        if (buyer->GetBuyCount() > 0) {
            left_buyers.push_back(buyer);
        }

        if (pq_head.idx + 1 < m_blocks[pq_head.first_expect].size()) {
            pq_head.idx++;
            pq_head.val = m_blocks[pq_head.first_expect][pq_head.idx]->GetBuyCount();
            Q.push(pq_head);
        }
    }

    log_debug("* left_buyers: %d", left_buyers.size());
}

void Buyer::do_assign_step2(vector<BGoods*>& left_buyers) {
    auto Seller = Seller::GetInstance();
    auto map_tol_stock = Seller->GetTolStock();
    auto tol_left_sellers = Seller->GetLeftGoods();

    vector<BGoods*> tmp_left_buyers = left_buyers;
    left_buyers.clear();

    for (auto& buyer : tmp_left_buyers) {
        auto& left_sellers = tol_left_sellers[buyer->GetBreed()];
        vector<vector<pair<string, string>>> permu_expects;
        vector<vector<int>> permu_expect_orders;

        buyer->GetPermutation(permu_expects, permu_expect_orders);

        struct pq_node {
            int score;
            HashMapItr seller_itr;
            int permu_idx;
            bool operator<(const pq_node& r) const { return score < r.score; }
        };
        priority_queue<pq_node> pq;

        for (int i = 0; i < permu_expects.size(); ++i) {
            sort(permu_expects[i].begin(), permu_expects[i].end());
            HashMapItr sellers_itr;
            int iRet = Seller->QueryGoods(sellers_itr, permu_expects[i], buyer->GetBreed());
            if (iRet != 0) continue;  // 找不到
            while (!sellers_itr->second.empty() && sellers_itr->second.front()->GetGoodStock() <= 0) {
                left_sellers.erase(sellers_itr->second.front());
                sellers_itr->second.pop_front();
            }
            if (sellers_itr->second.empty()) continue;
            int score = get_score(buyer, sellers_itr->second.front());
            pq.push(pq_node{score, sellers_itr, i});
        }

        while (buyer->GetBuyCount() > 0 && !pq.empty()) {
            auto head = pq.top();
            pq.pop();

            const auto seller = head.seller_itr->second.front();
            int val = min(buyer->GetBuyCount(), seller->GetGoodStock());

            buyer->SetBuyCount(buyer->GetBuyCount() - val);
            seller->SetGoodStock(seller->GetGoodStock() - val);

            Business bus(buyer, seller, val, permu_expect_orders[head.permu_idx]);
            m_results.push_back(bus);

            while (!head.seller_itr->second.empty() && head.seller_itr->second.front()->GetGoodStock() <= 0) {
                left_sellers.erase(head.seller_itr->second.front());
                head.seller_itr->second.pop_front();
            }

            if (!head.seller_itr->second.empty()) {
                int score = get_score(buyer, head.seller_itr->second.front());
                head.score = score;
                pq.push(head);
            }
        }

        if (buyer->GetBuyCount() > 0) {
            left_buyers.push_back(buyer);
        }
    }
    log_debug("* left_buyers: %d", left_buyers.size());
}

void Buyer::do_assign_step3(vector<BGoods*>& left_buyers) {
    auto Seller = Seller::GetInstance();
    auto map_tol_stock = Seller->GetTolStock();
    auto tol_left_sellers = Seller->GetLeftGoods();

    left_buyers.insert(left_buyers.end(), m_buyers.begin(), m_buyers.end());
    unordered_map<string, unordered_map<string, deque<SGoods*>>> ump_sellers;  // breed -> depot -> seller
    unordered_map<string, unordered_map<string, int>> ump_sellers_count;       // breed -> depot -> stock

    for (auto& [k, v] : tol_left_sellers) {
        for (auto& seller : v) {
            ump_sellers[k][seller->GetDepotID()].push_back(seller);
            ump_sellers_count[k][seller->GetDepotID()] += seller->GetGoodStock();
        }
    }

    sort(left_buyers.begin(), left_buyers.end(),
         [&](const BGoods* g1, const BGoods* g2) { return g1->GetBuyCount() > g2->GetBuyCount(); });

    for (auto& buyer : left_buyers) {
        auto& depot_sellers = ump_sellers[buyer->GetBreed()];

        while (buyer->GetBuyCount() > 0) {
            int min_value = INT_MAX;
            string min_depot_id = "";

            for (auto& [depot_id, sellers] : depot_sellers) {
                if (sellers.empty()) continue;
                int val = abs(buyer->GetBuyCount() - ump_sellers_count[buyer->GetBreed()][depot_id]);
                if (val < min_value) {
                    min_value = val;
                    min_depot_id = depot_id;
                }
            }

            while (buyer->GetBuyCount() > 0 && !depot_sellers[min_depot_id].empty()) {
                const auto seller = depot_sellers[min_depot_id].front();
                int val = min(buyer->GetBuyCount(), seller->GetGoodStock());
                buyer->SetBuyCount(buyer->GetBuyCount() - val);
                seller->SetGoodStock(seller->GetGoodStock() - val);
                ump_sellers_count[buyer->GetBreed()][min_depot_id] -= val;
                Business bus(buyer, seller, val, {0});
                m_results.push_back(bus);
                if (seller->GetGoodStock() <= 0) {
                    depot_sellers[min_depot_id].pop_front();
                }
            }
        }
    }

    for (auto& [k, v] : tol_left_sellers) {
        log_debug("* %s, left_sellers: %d", k.c_str(), v.size());
    }
}

void Buyer::assign_goods() {
    vector<BGoods*> left_buyers;
    this->do_assign_step1(left_buyers);
    this->do_assign_step2(left_buyers);
    this->do_assign_step3(left_buyers);
}
