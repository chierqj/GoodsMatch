#include "sgoods.h"

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