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

    this->read_data();            // 加载数据
    this->create_match_intent();  // 构造意向order
    this->assign_goods();         // 分配货物
    this->contact_result();       // 合并结果
    this->output();               // 输出结果
    this->debug();                // 打印输出

    log_info("* Buyer::Execute(): %.3fs", t.LogTime());
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
    log_info("----------------------------------------------");
    log_info("* 读文件");
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
        good->create_permutation();
        m_goods.push_back(good);
    }
    fin.close();

    log_info("* [load %s] [line: %d] [%.3fs]", file_path.c_str(), m_goods.size(), t.LogTime());
    log_info("----------------------------------------------");
}

void Buyer::contact_result() {
    log_info("----------------------------------------------");
    log_info("* 合并答案");
    ScopeTime t;

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
        for (int i = 0; i < it.buyer->GetIntents().size(); ++i) {
            const auto& expect = it.buyer->GetIntents()[i];
            if (expect.first.empty()) continue;
            if (ump[expect.first] == expect.second) {
                it.expect_order.push_back(i + 1);
            }
        }
    }

    log_info("* [time: %.3fs]", t.LogTime());
    log_info("----------------------------------------------");
}

vector<int> Buyer::get_intent_order(SGoods* seller, BGoods* buyer) {
    // string key = seller->GetGoodID() + "|" + buyer->GetBuyerID() + "|" + buyer->GetBreed();
    // return m_match_intent[key];

    const auto& depot_id = seller->GetDepotID();
    const auto& brand = seller->GetBrand();
    const auto& place = seller->GetPlace();
    const auto& year = seller->GetYear();
    const auto& level = seller->GetLevel();
    const auto& category = seller->GetCategory();

    vector<int> ans;
    for (int i = 0; i < buyer->GetIntents().size(); i++) {
        const auto& [k, v] = buyer->GetIntents()[i];
        if (v.empty()) continue;
        if (k == "仓库" && v == depot_id) ans.push_back(i);
        if (k == "品牌" && v == brand) ans.push_back(i);
        if (k == "产地" && v == place) ans.push_back(i);
        if (k == "年度" && v == year) ans.push_back(i);
        if (k == "等级" && v == level) ans.push_back(i);
        if (k == "类别" && v == category) ans.push_back(i);
    }
    return ans;
}

void Buyer::create_match_intent() {
    return;

    log_info("----------------------------------------------");
    log_info("* 构造意向顺序");
    ScopeTime t;

    auto Seller = Seller::GetInstance();
    for (auto& buyer : m_goods) {
        for (auto& depot : Seller->GetDepots()) {
            for (auto& [breed, items] : depot->sellers) {
                for (auto& item : items) {
                    const auto& seller = item.values.front();
                    const auto& depot_id = seller->GetDepotID();
                    const auto& brand = seller->GetBrand();
                    const auto& place = seller->GetPlace();
                    const auto& year = seller->GetYear();
                    const auto& level = seller->GetLevel();
                    const auto& category = seller->GetCategory();

                    vector<int> ans;
                    for (int i = 0; i < buyer->GetIntents().size(); i++) {
                        const auto& [k, v] = buyer->GetIntents()[i];
                        if (k == "仓库" && v == depot_id) ans.push_back(i);
                        if (k == "品牌" && v == brand) ans.push_back(i);
                        if (k == "产地" && v == place) ans.push_back(i);
                        if (k == "年度" && v == year) ans.push_back(i);
                        if (k == "等级" && v == level) ans.push_back(i);
                        if (k == "类别" && v == category) ans.push_back(i);
                    }

                    string key = item.good_id + "|" + buyer->GetBuyerID() + "|" + buyer->GetBreed();
                    m_match_intent[key] = ans;
                }
            }
        }
    }

    log_info("* [time: %.3fs]", t.LogTime());
    log_info("----------------------------------------------");
}