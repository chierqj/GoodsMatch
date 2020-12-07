/*
** EPITECH PROJECT, 2020
** GoodsMatch
** File description:
** buyer
*/

#ifndef BUYER_H_
#define BUYER_H_

#include <queue>
#include <unordered_map>
#include <vector>

#include "bgoods.h"
#include "business.h"
#include "src/seller/sgoods.h"
using namespace std;

class Buyer {
   public:
    static Buyer *GetInstance();

   public:
    void Execute();
    void debug();

   private:
    Buyer() {}
    Buyer(const Buyer &) = delete;
    Buyer &operator=(const Buyer &) = delete;

    void read_data();
    void output();
    void contact_result();

    void assign_buyer(BGoods *buyer, bool consider_first_intent);
    void assign_last_buyers();
    vector<vector<BGoods *>> create_buyers(int intent_id, int &buyers_count);
    void assign_goods();

   private:
    static Buyer *Instance;                // 单例
    vector<BGoods *> m_goods;              // buyers from file
    vector<Business> m_results;            // 成交
    vector<BGoods *> m_buyers_no_expects;  // no_expects
    vector<string> m_range_order;          // buyer遍历顺序
};

#endif /* !BUYER_H_ */
