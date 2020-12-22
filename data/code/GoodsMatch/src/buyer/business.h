/*
** EPITECH PROJECT, 2020
** GoodsMatch
** File description:
** business
*/

#ifndef BUSINESS_H_
#define BUSINESS_H_

#include <iostream>
#include <vector>

#include "bgoods.h"
#include "src/seller/sgoods.h"

using namespace std;

class Business {
   public:
    Business(BGoods *buyer, SGoods *seller, int assign_count, const vector<int> &expect_order)
        : buyer(buyer), seller(seller), assign_count(assign_count), expect_order(expect_order) {}

    inline string to_string();

   public:
    BGoods *buyer;             // 买家
    SGoods *seller;            // 卖家
    int assign_count;          // 成交额
    vector<int> expect_order;  // 对应意向顺序 [0 or 1-2-3]
};

inline string Business::to_string() {
    string res = "";
    res += buyer->GetBuyerID() + ",";
    res += seller->GetSellerID() + ",";
    res += buyer->GetBreed() + ",";
    res += seller->GetGoodID() + ",";
    res += seller->GetDepotID() + ",";
    res += std::to_string(assign_count) + ",";
    if (expect_order.empty()) {
        res += "0";
    } else {
        for (int i = 0; i < expect_order.size(); i++) {
            if (i != 0) {
                res += "-";
            }
            res += std::to_string(expect_order[i]);
        }
    }
    return res;
}

#endif /* !BUSINESS_H_ */
