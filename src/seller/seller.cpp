#include "seller.h"

#include <cassert>
#include <fstream>

#include "good_item.h"
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
    log_info("* 意向总数: %d", m_good_items.size());
    log_info("----------------------------------------------");
}

void Seller::Execute() {
    log_info("----------------------------------------------");
    log_info("* Seller 初始化");

    this->read_data();
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
        this->create_permutation(good);
        m_goods.push_back(good);
    }
    fin.close();

    log_info("* [load %s] [line: %d] [%.3fs]", file_path.c_str(), m_goods.size(), t.LogTime());
}

void Seller::create_hashmap() {
    ScopeTime t;

    sort(m_goods.begin(), m_goods.end(),
         [&](const SGoods* g1, const SGoods* g2) { return g1->GetGoodStock() > g2->GetGoodStock(); });

    unordered_map<string, int> ump_depot;  // depot_id -> depot;
    unordered_map<string, unordered_map<string, unordered_map<string, deque<SGoods*>>>> ump_sellers;
    unordered_map<string, unordered_map<string, unordered_map<string, int>>> ump_count;  // depot,breed,goodid

    for (const auto& seller : m_goods) {
        const auto& breed = seller->GetBreed();
        const auto& depot_id = seller->GetDepotID();
        const auto& good_id = seller->GetGoodID();
        ump_depot[depot_id] = 1;
        ump_sellers[depot_id][breed][good_id].push_back(seller);
        ump_count[depot_id][breed][good_id] += seller->GetGoodStock();
    }

    for (auto& [depot_id, depot] : ump_depot) {
        for (auto& [breed, sellers_gr] : ump_sellers[depot_id]) {
            for (auto& [good_id, sellers] : sellers_gr) {
                int tol = ump_count[depot_id][breed][good_id];
                auto item = new GoodItem(0, tol, good_id, sellers);
                for (auto& it : m_propoty[good_id]) {
                    if (it->values.size() == 1) m_good_items[it->map_key].push_back(item);
                    m_global_count[it->map_key] += tol;
                }
            }
        }
    }

    log_info("* [create hashmap] [%.3fs]", t.LogTime());
}

void Seller::create_permutation(SGoods* seller) {
    const auto& good_id = seller->GetGoodID();
    const auto& depot_id = seller->GetDepotID();
    const auto& brand = seller->GetBrand();
    const auto& price = seller->GetPlace();
    const auto& year = seller->GetYear();
    const auto& level = seller->GetLevel();
    const auto& category = seller->GetCategory();
    const auto& breed = seller->GetBreed();
    const auto& stock = seller->GetGoodStock();

    if (m_propoty.find(good_id) != m_propoty.end()) return;

    vector<string> names{"产地", "仓库", "品牌", "年度", "等级", "类别"};
    sort(names.begin(), names.end());
    unordered_map<string, string> ump;
    if (!depot_id.empty()) ump["仓库"] = depot_id;
    if (!brand.empty()) ump["品牌"] = brand;
    if (!price.empty()) ump["产地"] = price;
    if (!year.empty()) ump["年度"] = year;
    if (!level.empty()) ump["等级"] = level;
    if (!category.empty()) ump["类别"] = category;

    int sz = names.size();
    // 一个一个
    for (auto& name : names) {
        if (ump.find(name) == ump.end()) {
            continue;
        }
        string key = breed + "|" + name + "|" + ump[name];
        m_propoty[good_id].push_back(new Propoty({{name, ump[name]}}, key));
    }

    // 两个两个
    for (int i = 0; i < sz; ++i) {
        const auto& n1 = names[i];
        if (ump.find(n1) == ump.end()) continue;
        const auto& v1 = ump[n1];
        for (int j = i + 1; j < sz; ++j) {
            const auto& n2 = names[j];
            if (ump.find(n2) == ump.end()) continue;
            const auto& v2 = ump[n2];
            string key = breed + "|" + n1 + "|" + v1 + "|" + n2 + "|" + v2;
            vector<pair<string, string>> vt{{n1, v1}, {n2, v2}};
            m_propoty[good_id].push_back(new Propoty(vt, key));
        }
    }

    // 三个三个
    for (int i = 0; i < sz; ++i) {
        const auto& n1 = names[i];
        if (ump.find(n1) == ump.end()) continue;
        const auto& v1 = ump[n1];
        for (int j = i + 1; j < sz; ++j) {
            const auto& n2 = names[j];
            if (ump.find(n2) == ump.end()) continue;
            const auto& v2 = ump[n2];
            for (int k = j + 1; k < sz; ++k) {
                const auto& n3 = names[k];
                if (ump.find(n3) == ump.end()) continue;
                const auto& v3 = ump[n3];
                string key = breed + "|" + n1 + "|" + v1 + "|" + n2 + "|" + v2 + "|" + n3 + "|" + v3;
                vector<pair<string, string>> vt{{n1, v1}, {n2, v2}, {n3, v3}};
                m_propoty[good_id].push_back(new Propoty(vt, key));
            }
        }
    }

    // 四个四个
    for (int i = 0; i < sz; ++i) {
        const auto& n1 = names[i];
        if (ump.find(n1) == ump.end()) continue;
        const auto& v1 = ump[n1];
        for (int j = i + 1; j < sz; ++j) {
            const auto& n2 = names[j];
            if (ump.find(n2) == ump.end()) continue;
            const auto& v2 = ump[n2];
            for (int k = j + 1; k < sz; ++k) {
                const auto& n3 = names[k];
                if (ump.find(n3) == ump.end()) continue;
                const auto& v3 = ump[n3];
                for (int m = k + 1; m < sz; ++m) {
                    const auto& n4 = names[m];
                    if (ump.find(n4) == ump.end()) continue;
                    const auto& v4 = ump[n4];
                    string key =
                        breed + "|" + n1 + "|" + v1 + "|" + n2 + "|" + v2 + "|" + n3 + "|" + v3 + "|" + n4 + "|" + v4;
                    vector<pair<string, string>> vt{{n1, v1}, {n2, v2}, {n3, v3}, {n4, v4}};
                    m_propoty[good_id].push_back(new Propoty(vt, key));
                }
            }
        }
    }

    // 五个五个
    for (int i = 0; i < sz; ++i) {
        const auto& n1 = names[i];
        if (ump.find(n1) == ump.end()) continue;
        const auto& v1 = ump[n1];
        for (int j = i + 1; j < sz; ++j) {
            const auto& n2 = names[j];
            if (ump.find(n2) == ump.end()) continue;
            const auto& v2 = ump[n2];
            for (int k = j + 1; k < sz; ++k) {
                const auto& n3 = names[k];
                if (ump.find(n3) == ump.end()) continue;
                const auto& v3 = ump[n3];
                for (int m = k + 1; m < sz; ++m) {
                    const auto& n4 = names[m];
                    if (ump.find(n4) == ump.end()) continue;
                    const auto& v4 = ump[n4];
                    for (int n = m + 1; n < sz; ++n) {
                        const auto& n5 = names[n];
                        if (ump.find(n5) == ump.end()) continue;
                        const auto& v5 = ump[n5];
                        string key = breed + "|" + n1 + "|" + v1 + "|" + n2 + "|" + v2 + "|" + n3 + "|" + v3 + "|" +
                                     n4 + "|" + n5 + "|" + v5;
                        vector<pair<string, string>> vt{{n1, v1}, {n2, v2}, {n3, v3}, {n4, v4}, {n5, v5}};
                        m_propoty[good_id].push_back(new Propoty(vt, key));
                    }
                }
            }
        }
    }
}