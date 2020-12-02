#include "seller.h"

#include <cassert>
#include <fstream>

#include "src/comm/config.h"
#include "src/comm/log.h"
#include "src/comm/scope_time.h"
#include "src/comm/tools.h"

Seller* Seller::Instance = nullptr;
Seller* Seller::GetInstance() {
    if (Instance == nullptr) {
        Instance = new Seller();
    }
    return Instance;
}

void Seller::debug() {
    log_info("----------------------------------------------");
    log_info("* 货物总数: %d", m_goods.size());
    int tol_count = 0;
    for (auto& [k, v] : m_left_goods) tol_count += v.size();
    log_info("* 剩余货物: %d", tol_count);
    // for (auto& [k, v] : m_hashmap) log_info("* %s: %d", k.c_str(), v.size());
    log_info("* hashmap.keys: %d", m_hashmap.size());
    log_info("----------------------------------------------");
}

void Seller::Execute() {
    log_info("----------------------------------------------");
    log_info("* Seller 初始化");

    this->read_data();
    this->sort_goods();
    this->create_hashmap();
    this->debug();

    log_info("----------------------------------------------");
    return;
}

void Seller::read_data() {
    ScopeTime t;

    auto file_path = Config::g_conf["seller_file"];
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
        assert(row_data.size() == 10);
        auto good = new SGoods(row_data[0], row_data[1], row_data[2], atoi(row_data[3].c_str()), row_data[4],
                               row_data[5], row_data[6], row_data[7], row_data[8], row_data[9]);
        m_goods.push_back(good);
    }
    fin.close();

    log_info("* [load %s] [line: %d] [%.3fs]", file_path.c_str(), m_goods.size(), t.LogTime());
}

void Seller::sort_goods() {
    sort(m_goods.begin(), m_goods.end(),
         [&](const SGoods* g1, const SGoods* g2) { return g1->GetGoodStock() < g2->GetGoodStock(); });
}

void Seller::create_hashmap() {
    ScopeTime t;

    vector<string> names{"仓库", "品牌", "产地", "年度", "等级", "类别"};
    sort(names.begin(), names.end());

    for (const auto& seller : m_goods) {
        unordered_map<string, string> ump;
        const auto& depot_id = seller->GetDepotID();
        const auto& brand = seller->GetBrand();
        const auto& place = seller->GetPlace();
        const auto& year = seller->GetYear();
        const auto& level = seller->GetLevel();
        const auto& category = seller->GetCategory();
        ump["仓库"] = depot_id;
        ump["品牌"] = brand;
        ump["产地"] = place;
        ump["年度"] = year;
        ump["等级"] = level;
        ump["类别"] = category;

        const auto& breed = seller->GetBreed();
        int sz = names.size();

        // 一个一个
        for (auto& name : names) {
            m_hashmap[breed + "|" + name + "|" + ump[name]].push_back(seller);
            m_tol_stock[breed + "|" + name + "|" + ump[name]] += seller->GetGoodStock();
        }

        // 两个两个
        for (int i = 0; i < sz; ++i) {
            const auto& n1 = names[i];
            const auto& v1 = ump[n1];
            for (int j = i + 1; j < sz; ++j) {
                const auto& n2 = names[j];
                const auto& v2 = ump[n2];
                string key = breed + "|" + n1 + "|" + v1 + "|" + n2 + "|" + v2;
                m_hashmap[key].push_back(seller);
            }
        }

        // 三个三个
        for (int i = 0; i < sz; ++i) {
            const auto& n1 = names[i];
            const auto& v1 = ump[n1];
            for (int j = i + 1; j < sz; ++j) {
                const auto& n2 = names[j];
                const auto& v2 = ump[n2];
                for (int k = j + 1; k < sz; ++k) {
                    const auto& n3 = names[k];
                    const auto& v3 = ump[n3];
                    string key = breed + "|" + n1 + "|" + v1 + "|" + n2 + "|" + v2 + "|" + n3 + "|" + v3;
                    m_hashmap[key].push_back(seller);
                }
            }
        }

        // 四个四个
        for (int i = 0; i < sz; ++i) {
            const auto& n1 = names[i];
            const auto& v1 = ump[n1];
            for (int j = i + 1; j < sz; ++j) {
                const auto& n2 = names[j];
                const auto& v2 = ump[n2];
                for (int k = j + 1; k < sz; ++k) {
                    const auto& n3 = names[k];
                    const auto& v3 = ump[n3];
                    for (int m = k + 1; m < sz; ++m) {
                        const auto& n4 = names[m];
                        const auto& v4 = ump[n4];
                        string key = breed + "|" + n1 + "|" + v1 + "|" + n2 + "|" + v2 + "|" + n3 + "|" + v3 + "|" +
                                     n4 + "|" + v4;
                        m_hashmap[key].push_back(seller);
                    }
                }
            }
        }

        // 五个五个
        string key = breed;
        for (auto& name : names) key += "|" + name + "|" + ump[name];
        m_hashmap[key].push_back(seller);
        m_left_goods[breed].insert(seller);
    }

    for (auto& [k, sellers] : m_hashmap) {
        sort(sellers.begin(), sellers.end(), [&](const SGoods* g1, const SGoods* g2) {
            if (g1->GetDepotID() == g2->GetDepotID()) {
                return g1->GetGoodStock() > g2->GetGoodStock();
            }
            return g1->GetDepotID() > g2->GetDepotID();
        });
    }

    log_info("* [create hashmap] [%.3fs]", t.LogTime());
}

int Seller::QueryGoods(HashMapItr& result, const vector<pair<string, string>>& expect, const string& breed) {
    string key = breed;
    for (const auto& it : expect) key += "|" + it.first + "|" + it.second;
    auto it = m_hashmap.find(key);
    if (it == m_hashmap.end()) {
        return -1;
    }
    result = it;
    return 0;
}
