#include "depot.h"

#include "src/comm/log.h"

void Depot::pop_front(BGoods* buyer) {
    auto& items = sellers[buyer->GetBreed()];
    while (!items.empty()) {
        items.front().pop_front();
        if (items.front().values.empty()) {
            items.pop_front();
        } else {
            break;
        }
    }
}

void Depot::debug() {
    log_debug("* 仓库id: %s", depot_id.c_str());
    log_debug("* 货物种类: cf: %d, sr: %d, tol: %d", sellers["CF"].size(), sellers["SR"].size(),
              sellers["SR"].size() + sellers["CF"].size());
    for (auto& [breed, items] : sellers) {
        for (int i = 0; i < items.size(); ++i) {
            const auto& item = items[i];
            log_debug("* [%s, %d] [score: %d, tol_stock: %d, good_id: %s", breed.c_str(), i, item.score, item.tol_stock,
                      item.good_id.c_str());
        }
    }
}