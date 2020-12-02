#include "bgoods.h"

#include "src/comm/log.h"

void BGoods::debug() {
    log_debug("---------------------------------------");
    log_debug("* 买家id: %s", m_buyer_id.c_str());
    log_debug("* 持仓时间: %d", m_hold_time);
    log_debug("* 购买数量: %d", m_buy_count);
    log_debug("* 品种: %s", m_breed.c_str());
    int idx = 0;
    for (const auto& [fi, se] : m_excepts) {
        log_debug("* 志愿%d: (%s, %s)", ++idx, fi.c_str(), se.c_str());
    }
    log_debug("* ---------------------------------------");
}

void BGoods::GetPermutation(vector<vector<pair<string, string>>>& res_expects, vector<vector<int>>& expect_order) {
    res_expects.clear();
    expect_order.clear();

    int sz = m_excepts.size();
    bool sign = true;

    // 一个一个
    for (int i = 0; i < sz; ++i) {
        const auto& expect = m_excepts[i];
        if (expect.first.empty()) {
            sign = false;
            continue;
        }
        res_expects.push_back({expect});
        expect_order.push_back({i});
    }

    // 五个
    if (sign) {
        res_expects.push_back(m_excepts);
        expect_order.push_back({0, 1, 2, 3, 4});
    }

    // 两个两个
    for (int i = 0; i < sz; ++i) {
        const auto& exp1 = m_excepts[i];
        if (exp1.first.empty()) continue;
        for (int j = i + 1; j < sz; ++j) {
            const auto& exp2 = m_excepts[j];
            if (exp2.first.empty()) continue;
            res_expects.push_back({exp1, exp2});
            expect_order.push_back({i, j});
        }
    }

    //  三个三个
    for (int i = 0; i < sz; ++i) {
        const auto& exp1 = m_excepts[i];
        if (exp1.first.empty()) continue;
        for (int j = i + 1; j < sz; ++j) {
            const auto& exp2 = m_excepts[j];
            if (exp2.first.empty()) continue;
            for (int k = j + 1; k < sz; ++k) {
                const auto& exp3 = m_excepts[k];
                if (exp3.first.empty()) continue;
                res_expects.push_back({exp1, exp2, exp3});
                expect_order.push_back({i, j, k});
            }
        }
    }

    // 四个四个
    for (int i = 0; i < sz; ++i) {
        const auto& exp1 = m_excepts[i];
        if (exp1.first.empty()) continue;
        for (int j = i + 1; j < sz; ++j) {
            const auto& exp2 = m_excepts[j];
            if (exp2.first.empty()) continue;
            for (int k = j + 1; k < sz; ++k) {
                const auto& exp3 = m_excepts[k];
                if (exp3.first.empty()) continue;
                for (int m = k + 1; m < sz; ++m) {
                    const auto& exp4 = m_excepts[m];
                    if (exp4.first.empty()) continue;
                    res_expects.push_back({exp1, exp2, exp3, exp4});
                    expect_order.push_back({i, j, k, m});
                }
            }
        }
    }
}