#include "buyer.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <map>
#include <unordered_map>

#include "src/comm/config.h"
#include "src/comm/log.h"
#include "src/comm/scope_time.h"
#include "src/comm/tools.h"
#include "src/seller/seller.h"

void Buyer::Execute() {
    log_info("----------------------------------------------");
    log_info("* Buyer 开始运行");

    this->read_data();
    this->assign_goods();
    this->output();
    this->debug();

    log_info("----------------------------------------------");
    return;
}

void Buyer::debug() {
    int ok = 0, wa = 0;
    for (auto& it : m_goods) {
        if (it->GetBuyCount() > 0) {
            ++wa;
        } else {
            ++ok;
        }
    }

    unordered_map<string, int> count_depot;
    for (auto& it : m_results) {
        count_depot[it.buyer_id + "|" + it.breed + "|" + it.depot_id]++;
    }

    map<int, int> count;
    for (auto& [k, v] : count_depot) {
        count[v]++;
    }

    int tol = 0;
    for (auto& [k, v] : count) {
        log_info("* 仓库个数: %d, 成交条数: %d", k, v);
        tol += k * v;
    }
    log_info("* tol: %d", tol);

    log_info("----------------------------------------------");
    log_info("* 货物总数: %d", m_goods.size());
    log_info("* 成交数目: %d", ok);
    log_info("* 失败数目: %d", wa);
    log_info("* 成交明细: %d", m_results.size());
    log_info("----------------------------------------------");
}

void Buyer::output() {
    auto file_path = Config::g_conf["result_file"];
    ofstream fout(file_path);
    fout << "买方客户,卖方客户,品种,货物编号,仓库,分配货物数量,对应意向顺序\n";
    for (auto& it : m_results) {
        fout << it.to_string() << "\n";
    }
    fout.close();
}

void Buyer::read_data() {
    ScopeTime t;

    auto file_path = Config::g_conf["buyer_file"];
    ifstream fin(file_path);
    string line;
    bool fir = true;
    int count = 0;
    while (fin >> line) {
        if (fir) {
            fir = false;
            continue;
        }
        auto row_data = Tools::Split(line, ",");
        assert(row_data.size() == 14);

        vector<pair<string, string>> excepts{{row_data[4], row_data[5]},
                                             {row_data[6], row_data[7]},
                                             {row_data[8], row_data[9]},
                                             {row_data[10], row_data[11]},
                                             {row_data[12], row_data[13]}};

        auto good = new BGoods(row_data[0], atoi(row_data[1].c_str()), atoi(row_data[2].c_str()), row_data[3], excepts);
        m_goods.push_back(good);
    }
    fin.close();

    log_info("* [load %s] [line: %d] [%.3fs]", file_path.c_str(), m_goods.size(), t.LogTime());
}

void Buyer::do_assign(SGoods* seller, BGoods* buyer, int except_id) {
    if (seller->GetGoodStock() <= 0) {
        return;
    }
    int val = min(seller->GetGoodStock(), buyer->GetBuyCount());
    seller->SetGoodStock(seller->GetGoodStock() - val);
    buyer->SetBuyCount(buyer->GetBuyCount() - val);

    Business bus(buyer->GetBuyerID(), seller->GetSellerID(), seller->GetBreed(), seller->GetGoodID(),
                 seller->GetDepotID(), val, {except_id});

    m_results.push_back(bus);
}

void Buyer::assign_goods() {
    sort(m_goods.begin(), m_goods.end(), [&](const BGoods* g1, const BGoods* g2) {
        if (g1->GetBreed() != g2->GetBreed()) {
            return g1->GetBreed() > g2->GetBreed();
        }
        if (g1->GetExcepts()[0] == g2->GetExcepts()[0]) {
            return g1->GetHoldTime() > g2->GetHoldTime();
        }
        return g1->GetExcepts()[0] > g2->GetExcepts()[0];
    });

    auto Seller = Seller::GetInstance();
    auto left_sellers = Seller->GetLeftGoods();

    for (auto& buyer : m_goods) {
        // 先按照意向分配
        for (int except_id = 0; except_id < 5 && buyer->GetBuyCount() > 0; ++except_id) {
            HashMapItr result;
            int iRet = Seller->GetGoods(result, buyer->GetExcepts()[except_id], buyer->GetBreed());
            if (iRet != 0) continue;
            while (!result->second.empty() && buyer->GetBuyCount() > 0) {
                auto seller = result->second.front();
                this->do_assign(seller, buyer, except_id + 1);
                if (seller->GetGoodStock() <= 0) {  // 库存为0，需要删除
                    result->second.pop_front();
                    left_sellers.erase(seller);
                }
            }
        }
        // 没分完，随便分
        while (!left_sellers.empty() && buyer->GetBuyCount() > 0) {
            auto seller = *left_sellers.begin();
            this->do_assign(seller, buyer, 0);
            if (seller->GetGoodStock() <= 0) {  // 库存为0，需要删除
                left_sellers.erase(seller);
            }
        }
    }
}