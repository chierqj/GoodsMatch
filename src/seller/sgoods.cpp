#include "sgoods.h"

#include <algorithm>
#include <unordered_map>

#include "src/comm/log.h"

void SGoods::debug() {
    log_debug("* ---------------------------------------");
    log_debug("* 卖家id: %s", m_seller_id.c_str());
    log_debug("* 品种: %s", m_breed.c_str());
    log_debug("* 货物id: %s", m_good_id.c_str());
    log_debug("* 库存: %d", m_good_stock);
    log_debug("* 仓库id: %s", m_depot_id.c_str());
    log_debug("* 品牌: %s", m_brand.c_str());
    log_debug("* 产地: %s", m_place.c_str());
    log_debug("* 年度: %s", m_year.c_str());
    log_debug("* 等级: %s", m_level.c_str());
    log_debug("* 类别: %s", m_category.c_str());
    log_debug("* ---------------------------------------");
}

void SGoods::create_permutation() {
    vector<string> names{"产地", "仓库", "品牌", "年度", "等级", "类别"};

    unordered_map<string, string> ump;
    if (!m_depot_id.empty()) ump["仓库"] = m_depot_id;
    if (!m_brand.empty()) ump["品牌"] = m_brand;
    if (!m_place.empty()) ump["产地"] = m_place;
    if (!m_year.empty()) ump["年度"] = m_year;
    if (!m_level.empty()) ump["等级"] = m_level;
    if (!m_category.empty()) ump["类别"] = m_category;

    int sz = names.size();
    // 一个一个
    bool sign = true;
    for (auto& name : names) {
        if (ump.find(name) == ump.end()) {
            sign = false;
            continue;
        }
        string key = m_breed + "|" + name + "|" + ump[name];
        m_propoty.push_back(new Propoty({{name, ump[name]}}, key));
        // m_propoty.push_back(new Propoty(key));
    }

    // 五个五个
    // if (sign) {
    //     vector<pair<string, string>> vt;
    //     bool fir = true;
    //     string key = m_breed;
    //     for (auto& name : names) {
    //         key += "|" + name + "|" + ump[name];
    //         vt.push_back({name, ump[name]});
    //     }
    //     m_propoty.push_back(new Propoty(vt, key));
    //     // m_propoty.push_back(new Propoty(key));
    // }

    // 两个两个
    for (int i = 0; i < sz; ++i) {
        const auto& n1 = names[i];
        if (ump.find(n1) == ump.end()) continue;
        const auto& v1 = ump[n1];
        for (int j = i + 1; j < sz; ++j) {
            const auto& n2 = names[j];
            if (ump.find(n2) == ump.end()) continue;
            const auto& v2 = ump[n2];
            string key = m_breed + "|" + n1 + "|" + v1 + "|" + n2 + "|" + v2;
            vector<pair<string, string>> vt{{n1, v1}, {n2, v2}};
            m_propoty.push_back(new Propoty(vt, key));
            // m_propoty.push_back(new Propoty(key));
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
                string key = m_breed + "|" + n1 + "|" + v1 + "|" + n2 + "|" + v2 + "|" + n3 + "|" + v3;
                vector<pair<string, string>> vt{{n1, v1}, {n2, v2}, {n3, v3}};
                m_propoty.push_back(new Propoty(vt, key));
                // m_propoty.push_back(new Propoty(key));
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
                        m_breed + "|" + n1 + "|" + v1 + "|" + n2 + "|" + v2 + "|" + n3 + "|" + v3 + "|" + n4 + "|" + v4;
                    vector<pair<string, string>> vt{{n1, v1}, {n2, v2}, {n3, v3}, {n4, v4}};
                    m_propoty.push_back(new Propoty(vt, key));
                    // m_propoty.push_back(new Propoty(key));
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
                        string key = m_breed + "|" + n1 + "|" + v1 + "|" + n2 + "|" + v2 + "|" + n3 + "|" + v3 + "|" +
                                     n4 + "|" + n5 + "|" + v5;
                        vector<pair<string, string>> vt{{n1, v1}, {n2, v2}, {n3, v3}, {n4, v4}, {n5, v5}};
                        m_propoty.push_back(new Propoty(vt, key));
                        // m_propoty.push_back(new Propoty(key));
                    }
                }
            }
        }
    }
}