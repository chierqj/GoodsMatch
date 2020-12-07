#include "bgoods.h"

#include <algorithm>

#include "src/comm/log.h"

void BGoods::debug() {
    log_debug("---------------------------------------");
    log_debug("* 买家id: %s", m_buyer_id.c_str());
    log_debug("* 持仓时间: %d", m_hold_time);
    log_debug("* 购买数量: %d", m_buy_count);
    log_debug("* 品种: %s", m_breed.c_str());
    int idx = 0;
    for (const auto& [fi, se] : m_intent) {
        log_debug("* 志愿%d: (%s, %s)", ++idx, fi.c_str(), se.c_str());
    }
    log_debug("* ---------------------------------------");
}

void BGoods::create_permutation() {
    unordered_map<string, vector<int>> breed_scores;
    breed_scores["CF"] = vector<int>{33, 27, 20, 13, 7};
    breed_scores["SR"] = vector<int>{40, 30, 20, 10, 0};

    int sz = m_intent.size();
    bool sign = true;
    m_useful_intent = 0;

    m_intent_map_key = vector<string>(m_intent.size(), "");
    for (int i = 0; i < m_intent.size(); i++) {
        if (!m_intent[i].first.empty()) {
            string key = m_breed + "|" + m_intent[i].first + "|" + m_intent[i].second;
            m_intent_map_key[i] = key;
        }
    }

    // 一个一个
    for (int i = 0; i < sz; ++i) {
        const auto& expect = m_intent[i];
        if (expect.first.empty()) {
            sign = false;
            continue;
        }
        ++m_useful_intent;
        auto exp = new Intent({expect}, {i});
        exp->init(m_breed);
        m_permu_intents.push_back(exp);
    }

    // 五个
    if (sign) {
        auto exp = new Intent({m_intent}, {0, 1, 2, 3, 4});
        exp->init(m_breed);
        m_permu_intents.push_back(exp);
    }

    // 两个两个
    for (int i = 0; i < sz; ++i) {
        const auto& exp1 = m_intent[i];
        if (exp1.first.empty()) continue;
        for (int j = i + 1; j < sz; ++j) {
            const auto& exp2 = m_intent[j];
            if (exp2.first.empty()) continue;

            auto exp = new Intent({exp1, exp2}, {i, j});
            exp->init(m_breed);
            m_permu_intents.push_back(exp);
        }
    }

    //  三个三个
    for (int i = 0; i < sz; ++i) {
        const auto& exp1 = m_intent[i];
        if (exp1.first.empty()) continue;
        for (int j = i + 1; j < sz; ++j) {
            const auto& exp2 = m_intent[j];
            if (exp2.first.empty()) continue;
            for (int k = j + 1; k < sz; ++k) {
                const auto& exp3 = m_intent[k];
                if (exp3.first.empty()) continue;

                auto exp = new Intent({exp1, exp2, exp3}, {i, j, k});
                exp->init(m_breed);
                m_permu_intents.push_back(exp);
            }
        }
    }

    // 四个四个
    for (int i = 0; i < sz; ++i) {
        const auto& exp1 = m_intent[i];
        if (exp1.first.empty()) continue;
        for (int j = i + 1; j < sz; ++j) {
            const auto& exp2 = m_intent[j];
            if (exp2.first.empty()) continue;
            for (int k = j + 1; k < sz; ++k) {
                const auto& exp3 = m_intent[k];
                if (exp3.first.empty()) continue;
                for (int m = k + 1; m < sz; ++m) {
                    const auto& exp4 = m_intent[m];
                    if (exp4.first.empty()) continue;
                    auto exp = new Intent({exp1, exp2, exp3, exp4}, {i, j, k, m});
                    exp->init(m_breed);
                    m_permu_intents.push_back(exp);
                }
            }
        }
    }

    sort(m_permu_intents.begin(), m_permu_intents.end(),
         [&](const Intent* i1, const Intent* i2) { return i1->score > i2->score; });
}