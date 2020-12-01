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
    log_info("* 剩余货物: %d", m_left_goods.size());
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

    for (auto it : m_goods) {
        auto breed = it->GetBreed();
        m_hashmap[breed + "仓库"][it->GetDepotID()].push_back(it);
        m_hashmap[breed + "品牌"][it->GetBrand()].push_back(it);
        m_hashmap[breed + "产地"][it->GetPlace()].push_back(it);
        m_hashmap[breed + "年度"][it->GetYear()].push_back(it);
        m_hashmap[breed + "等级"][it->GetLevel()].push_back(it);
        m_hashmap[breed + "类别"][it->GetCategory()].push_back(it);
        m_left_goods.insert(it);
    }

    log_info("* [create hashmap] [%.3fs]", t.LogTime());
}

int Seller::GetGoods(HashMapItr& result, const pair<string, string>& expect, const string& breed) {
    auto it = m_hashmap.find(breed + expect.first);
    if (it == m_hashmap.end()) {
        return -1;
    }
    auto res = it->second.find(expect.second);
    if (res == it->second.end()) {
        return -1;
    }
    result = res;
    return 0;
}

void Seller::EraseGoods(SGoods* goods) { m_left_goods.erase(goods); }