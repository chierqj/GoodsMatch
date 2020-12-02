/*
** EPITECH PROJECT, 2020
** GoodsMatch
** File description:
** buyer
*/

#ifndef BUYER_H_
#define BUYER_H_

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
    void assign_goods();
    void output();
    void contact_result();
    void pretreat();

    void do_assign_step1(vector<BGoods *> &left_buyers);
    void do_assign_step2(vector<BGoods *> &left_buyers);
    void do_assign_step3(vector<BGoods *> &left_buyers);

   private:
    static Buyer *Instance;                            // 单例
    vector<BGoods *> m_goods;                          // buyers from file
    unordered_map<string, BGoods *> m_hash_goods;      // k: buyer_id+breed
    vector<Business> m_results;                        // 成交
    unordered_map<string, vector<BGoods *>> m_blocks;  // [greed+expect1]
    vector<BGoods *> m_buyers;                         // buyers for assign
};

#endif /* !BUYER_H_ */
