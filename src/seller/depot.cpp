#include "depot.h"

#include "src/comm/log.h"

void Depot::debug() {
    log_debug("---------------------------------------------------------------");
    log_debug("* 仓库id: %s", depot_id.c_str());
    log_debug("* 货物种类: cf: %d, sr: %d, tol: %d", sellers["CF"].size(), sellers["SR"].size(),
              sellers["SR"].size() + sellers["CF"].size());
    for (auto& [intent, items] : map_sellers) {
        log_debug("* [意向: %s] [货物种类数: %d] [总库存: %d]", intent.c_str(), items.size(), map_stock[intent]);
    }
}