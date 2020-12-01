/*
** EPITECH PROJECT, 2020
** GoodsMatch
** File description:
** buyer
*/

#ifndef BUYER_H_
#define BUYER_H_

#include <vector>

#include "bgoods.h"
#include "src/seller/sgoods.h"
using namespace std;

struct Business {
    Business(const string &buyer_id, const string &seller_id, const string &breed, const string &good_id,
             const string &depot_id, int assign_count, const vector<int> &expect_order)
        : buyer_id(buyer_id),
          seller_id(seller_id),
          breed(breed),
          good_id(good_id),
          depot_id(depot_id),
          assign_count(assign_count),
          expect_order(expect_order) {}

    string to_string() {
        string res = "";
        res += buyer_id + ",";
        res += seller_id + ",";
        res += breed + ",";
        res += good_id + ",";
        res += depot_id + ",";
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

    string buyer_id;           // 买方客户
    string seller_id;          // 卖方客户
    string breed;              // 品种
    string good_id;            // 货物编号
    string depot_id;           // 仓库
    int assign_count;          // 分配货物数量
    vector<int> expect_order;  // 对应意向顺序 [0 or 1-2-3]
};

class Buyer {
   public:
    void Execute();
    void debug();

   private:
    void read_data();
    void do_assign(SGoods *seller, BGoods *buyer, int expect_id);
    void assign_goods();
    void output();

   private:
    vector<BGoods *> m_goods;
    vector<Business> m_results;
};

#endif /* !BUYER_H_ */
