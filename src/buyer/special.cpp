#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <fstream>
#include <map>
#include <queue>
#include <random>
#include <set>

#include "buyer.h"
#include "src/comm/config.h"
#include "src/comm/debug.h"
#include "src/comm/log.h"
#include "src/comm/scope_time.h"
#include "src/comm/tools.h"
#include "src/seller/seller.h"

// 100, size -> 157.935
// 300, size -> 157.936
// 1000, size -> 157.941
// 2000, size -> 157.945
// 6000, size -> 157.946
// 10000, size ->
void Buyer::assign_special(BGoods* buyer, int intent_id) {
    // if (buyer->GetBreed() == "CF") return;
    static int max_count = 0;
    if (max_count++ > 10000) return;

    auto Seller = Seller::GetInstance();

    auto& permu_intenters = buyer->GetPermuIntents();
    sort(permu_intenters.begin(), permu_intenters.end(),
         [&](const Intent* g1, const Intent* g2) { return g1->score > g2->score; });

    for (auto& intenter : permu_intenters) {
        if (buyer->GetBuyCount() <= 0) break;
        if (intent_id == 0 && intenter->orders[0] != 0) continue;

        vector<Depot*> depots;
        for (auto& depot : Seller->GetDepots()) {
            depots.push_back(depot);
        }
        sort(depots.begin(), depots.end(), [&](Depot* depot1, Depot* depot2) {
            int count1 = depot1->map_stock[intenter->map_key];
            int count2 = depot2->map_stock[intenter->map_key];
            return count1 > count2;
        });

        for (int i = 0; i < depots.size(); i++) {
            auto& depot = depots[i];
            if (buyer->GetBuyCount() <= 0) break;

            auto& items = depot->map_sellers[intenter->map_key];
            sort(items.begin(), items.end(),
                 [&](const Depot::Item* it1, const Depot::Item* it2) { return it1->tol_stock > it2->tol_stock; });
            pop_depot(items);

            while (buyer->GetBuyCount() > 0 && !items.empty()) {
                auto& item = items.front();
                item->pop_front();

                while (buyer->GetBuyCount() > 0 && !item->values.empty()) {
                    auto& seller = item->values.front();
                    this->do_business(depot, item, buyer, seller);
                    item->pop_front();
                }
                pop_depot(items);
            }
        }
    }
}
