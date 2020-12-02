#include "buyer.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <fstream>
#include <map>
#include <set>

#include "src/comm/config.h"
#include "src/comm/debug.h"
#include "src/comm/log.h"
#include "src/comm/scope_time.h"
#include "src/comm/tools.h"
#include "src/seller/seller.h"

Buyer* Buyer::Instance = nullptr;
Buyer* Buyer::GetInstance() {
    if (Instance == nullptr) {
        Instance = new Buyer();
    }
    return Instance;
}

void Buyer::Execute() {
    log_info("----------------------------------------------");
    log_info("* Buyer 开始运行");
    ScopeTime t;

    this->read_data();  // 加载数据
    log_info("* 加载数据: %.3fs", t.LogTime());

    this->pretreat();  // 预处理
    log_info("* 预处理: %.3fs", t.LogTime());

    this->assign_goods();  // 分配货物
    log_info("* 分配货物: %.3fs", t.LogTime());

    this->contact_result();  // 合并结果
    log_info("* 合并结果: %.3fs", t.LogTime());

    this->output();  // 输出结果
    log_info("* 输出结果: %.3fs", t.LogTime());

    this->debug();  // 打印输出

    log_info("----------------------------------------------");
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
        count_depot[it.buyer->GetBuyerID() + "|" + it.buyer->GetBreed() + "|" + it.seller->GetDepotID()]++;
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
        auto k = good->GetBuyerID() + "|" + good->GetBreed();
        m_hash_goods[k] = good;
    }
    fin.close();
}

void Buyer::contact_result() {
    unordered_map<string, vector<Business>> mp_result;
    int max_count = 0;

    for (auto& it : m_results) {
        string k = it.buyer->GetBuyerID() + "|" + it.seller->GetSellerID() + "|" + it.buyer->GetBreed() + "|" +
                   it.seller->GetGoodID();
        mp_result[k].push_back(it);
        max_count = max(max_count, (int)mp_result[k].size());
    }

    m_results.clear();
    for (auto& [k, v] : mp_result) {
        Business bus = v.front();
        for (int i = 1; i < v.size(); ++i) {
            bus.assign_count += v[i].assign_count;
        }
        m_results.push_back(bus);
    }

    for (auto& it : m_results) {
        unordered_map<string, string> ump;
        ump["仓库"] = it.seller->GetDepotID();
        ump["品牌"] = it.seller->GetBrand();
        ump["产地"] = it.seller->GetPlace();
        ump["年度"] = it.seller->GetYear();
        ump["等级"] = it.seller->GetLevel();
        ump["类别"] = it.seller->GetCategory();
        it.expect_order.clear();
        for (int i = 0; i < it.buyer->GetExcepts().size(); ++i) {
            const auto& expect = it.buyer->GetExcepts()[i];
            if (expect.first.empty()) continue;
            if (ump[expect.first] == expect.second) {
                it.expect_order.push_back(i + 1);
            }
        }
    }
}

void Buyer::pretreat() {
    int max_count = 0;
    for (auto& goods : m_goods) {
        if (goods->GetExcepts()[0].first.empty()) {
            m_buyers.push_back(goods);
            continue;
        }
        string k = goods->GetBreed() + "|" + goods->GetExcepts()[0].first + "|" + goods->GetExcepts()[0].second;
        m_blocks[k].push_back(goods);
        max_count = max(max_count, (int)m_blocks[k].size());
    }

    int tol_count = 0;
    auto Seller = Seller::GetInstance();

    for (auto& [k, buyers] : m_blocks) {
        sort(buyers.begin(), buyers.end(), [&](const BGoods* g1, const BGoods* g2) {
            if (g1->GetHoldTime() == g2->GetHoldTime()) {
                return g1->GetBuyCount() > g2->GetBuyCount();
            }
            return g1->GetHoldTime() > g2->GetHoldTime();
        });

        auto buyer = buyers.front();
        vector<int> seller_count;

        int sum_count = 0;
        for (int i = 0; i < buyer->GetExcepts().size(); ++i) {
            const auto& excepct = buyer->GetExcepts()[i];
            HashMapItr tmp;
            int iRet = Seller->QueryGoods(tmp, {excepct}, buyer->GetBreed());
            if (iRet != 0) {
                seller_count.push_back(0);
                continue;
            }
            seller_count.push_back(tmp->second.size());
            sum_count += tmp->second.size();
        }

        log_debug("* [%s] [买: %d, 卖: (%d,%d,%d,%d,%d)]", k.c_str(), buyers.size(), seller_count[0], seller_count[1],
                  seller_count[2], seller_count[3], seller_count[4]);

        tol_count += buyers.size();
    }

    log_debug("* max_buy_count: %d", max_count);
    log_debug("* buyer.size(): %d", m_buyers.size());
    log_debug("* tol_count: %d", tol_count + m_buyers.size());
}