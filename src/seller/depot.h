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

    void pop_front(BGoods* buyer);
    void debug();

   public:
    string depot_id;
    struct Item {
        void pop_front() {
            while (!values.empty() && values.front()->GetGoodStock() <= 0) {
                values.pop_front();
            }
        }
        int score;              // 得分
        int tol_stock;          // 总库存
        string good_id;         // 货物编号
        deque<SGoods*> values;  // 货物items
    };
    unordered_map<string, deque<Item>> sellers;  // key=品种，values groupby_goodID
};

#endif /* !DEPOT_H_ */
