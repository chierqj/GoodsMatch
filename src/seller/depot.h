/*
** EPITECH PROJECT, 2020
** GoodsMatch
** File description:
** depot
*/

#ifndef DEPOT_H_
#define DEPOT_H_

#include <algorithm>
#include <cassert>
#include <deque>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "sgoods.h"
#include "src/buyer/bgoods.h"
using namespace std;

class Depot {
   public:
    Depot(const string& depot_id) : depot_id(depot_id) {}

    void debug();

   public:
    struct Item {
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
        int score;              // 得分
        int tol_stock;          // 总库存
        string good_id;         // 货物编号
        deque<SGoods*> values;  // 货物items
    };

    string depot_id;                                  // 仓库编号
    unordered_map<string, deque<Item*>> sellers;      // key=品种，values groupby_goodID
    unordered_map<string, deque<Item*>> map_sellers;  // 品种+意向组合
    unordered_map<string, int> map_stock;             // 品种+意向组合
};

#endif /* !DEPOT_H_ */
