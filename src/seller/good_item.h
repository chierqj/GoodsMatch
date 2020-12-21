/*
** EPITECH PROJECT, 2020
** GoodsMatch
** File description:
** good_item
*/

#ifndef GOOD_ITEM_H_
#define GOOD_ITEM_H_
#include <algorithm>
#include <cassert>
#include <deque>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "sgoods.h"
#include "src/buyer/bgoods.h"
#include "src/comm/log.h"

using namespace std;

class GoodItem {
   public:
    GoodItem(int score, int tol_stock, string good_id, deque<SGoods*> values)
        : score(score), tol_stock(tol_stock), good_id(good_id), values(values) {}

    void pop_front() {
        while (!values.empty() && values.front()->GetGoodStock() <= 0) {
            values.pop_front();
        }
    }
    string to_string() {
        string res = "";
        res += "货物编号: " + good_id;
        res += ", 总库存: " + std::to_string(tol_stock);
        res += ", 货物数目: " + std::to_string(values.size());
        res += ", 得分: " + std::to_string(score);
        return res;
    }
    bool empty() { return values.empty(); }
    void debug() { log_debug("* %s", to_string().c_str()); }

    double score;           // 得分
    int tol_stock;          // 总库存
    string good_id;         // 货物编号
    deque<SGoods*> values;  // 货物items
};

#endif /* !GOOD_ITEM_H_ */
