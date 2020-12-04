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
            m_global_count[propoty->map_key] += seller->GetGoodStock();
        }

        depot->useful_sellers[seller->GetBreed()].insert(seller);
    }

    log_info("* [create hashmap] [%.3fs]", t.LogTime());
}
