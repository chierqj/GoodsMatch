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
    m_intent_map_key = vector<string>(m_intent.size(), "");
    for (int i = 0; i < m_intent.size(); i++) {
        if (!m_intent[i].first.empty()) {
            string key = m_breed + "|" + m_intent[i].first + "|" + m_intent[i].second;
            m_intent_map_key[i] = key;
        }
    }
}