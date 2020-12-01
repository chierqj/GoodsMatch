#include "record.h"

#include "src/comm/log.h"

void Record::debug() {
    log_debug("买家id: %s", m_buyer_id.c_str());
    log_debug("卖家id: %s", m_seller_id.c_str());
    log_debug("品种: %s", m_breed.c_str());
    log_debug("货物id: %s", m_good_id.c_str());
    log_debug("仓库id: %s", m_depot_id.c_str());
    log_debug("货物数量: %d", m_good_stock);
    log_debug("满足意向: %s", m_intent.c_str());
}