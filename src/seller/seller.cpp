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
    log_info("* depot_count: %d", m_sellers_groupby_depot.size());
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
        good->create_permutation();
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
        m_sellers_groupby_depot[seller->GetDepotID()] = new Depot(seller->GetDepotID());
    }

    for (const auto& seller : m_goods) {
        auto& depot = m_sellers_groupby_depot[seller->GetDepotID()];

        for (const auto& propoty : seller->GetPropoty()) {
            string key = propoty->map_key;
            depot->sellers_groupby_except[key].push_back(seller);
            depot->sellers_tol_stock[key] += seller->GetGoodStock();

            if (propoty->values.size() == 1) {
                m_global_count[propoty->map_key] += seller->GetGoodStock();
            }
        }

        depot->useful_sellers[seller->GetBreed()].insert(seller);
    }

    // for (auto& [depot_id, depot] : m_sellers_groupby_depot) {
    //     for (auto& [intent, sellers] : depot->sellers_groupby_except) {
    //         auto vct = Tools::Split(intent, "|");
    //         unordered_set<string> vis;

    //         for (int i = 0; i < vct.size(); i += 2) {
    //             string key = vct[i] + "|" + vct[i + 1];
    //             vis.insert(key);
    //         }

    //         sort(sellers.begin(), sellers.end(), [&](const SGoods* g1, const SGoods* g2) {
    //             int count1 = 0, count2 = 0;
    //             int min_value1 = INT_MAX, min_value2 = INT_MAX;

    //             {
    //                 unordered_map<string, string> ump;
    //                 const auto& depot_id = g1->GetDepotID();
    //                 const auto& brand = g1->GetBrand();
    //                 const auto& place = g1->GetPlace();
    //                 const auto& year = g1->GetYear();
    //                 const auto& level = g1->GetLevel();
    //                 const auto& category = g1->GetCategory();

    //                 if (!depot_id.empty()) ump["仓库"] = depot_id;
    //                 if (!brand.empty()) ump["品牌"] = brand;
    //                 if (!place.empty()) ump["产地"] = place;
    //                 if (!year.empty()) ump["年度"] = year;
    //                 if (!level.empty()) ump["等级"] = level;
    //                 if (!category.empty()) ump["类别"] = category;

    //                 for (auto& name : names) {
    //                     string key = name + "|" + ump[name];
    //                     if (vis.find(key) == vis.end()) {
    //                         ++count1;
    //                         min_value1 = min(min_value1, m_global_count[key]);
    //                     }
    //                 }
    //             }

    //             {
    //                 unordered_map<string, string> ump;
    //                 const auto& depot_id = g2->GetDepotID();
    //                 const auto& brand = g2->GetBrand();
    //                 const auto& place = g2->GetPlace();
    //                 const auto& year = g2->GetYear();
    //                 const auto& level = g2->GetLevel();
    //                 const auto& category = g2->GetCategory();

    //                 if (!depot_id.empty()) ump["仓库"] = depot_id;
    //                 if (!brand.empty()) ump["品牌"] = brand;
    //                 if (!place.empty()) ump["产地"] = place;
    //                 if (!year.empty()) ump["年度"] = year;
    //                 if (!level.empty()) ump["等级"] = level;
    //                 if (!category.empty()) ump["类别"] = category;

    //                 for (auto& name : names) {
    //                     string key = name + "|" + ump[name];
    //                     if (vis.find(key) == vis.end()) {
    //                         ++count2;
    //                         min_value2 = min(min_value2, m_global_count[key]);
    //                     }
    //                 }
    //             }

    //             if (count1 == count2) return min_value1 > min_value2;
    //             return count1 < count2;
    //         });

    //         for (int i = 0; i < min(10, (int)sellers.size()); ++i) {
    //             sellers[i]->debug();
    //         }
    //     }
    // }

    log_info("* [create hashmap] [%.3fs]", t.LogTime());
}
