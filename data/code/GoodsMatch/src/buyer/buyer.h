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
#include "src/seller/good_item.h"
#include "src/seller/sgoods.h"
using namespace std;

class Buyer {
   public:
    static Buyer *GetInstance();

   public:
    void Execute();
    void debug();

   private:
    template <typename T>
    void pop_deque(deque<T> &items) {
        while (!items.empty()) {
            items.front()->pop_front();
            if (items.front()->empty()) {
                items.pop_front();
            } else {
                break;
            }
        }
    }

    Buyer() {}
    Buyer(const Buyer &) = delete;
    Buyer &operator=(const Buyer &) = delete;

    void read_data();
    vector<int> get_intent_order(SGoods *seller, BGoods *buyer);
    int get_intent_score(SGoods *seller, BGoods *buyer);
    void output();
    void contact_result();

    void do_business(GoodItem *item, BGoods *buyer, SGoods *seller, int intent_id);
    void assign_buyer(BGoods *buyer, int intent_id);
    void assign_last_buyers();
    vector<pair<string, deque<BGoods *>>> create_buyers(int intent_id, int &buyers_count,
                                                        unordered_map<string, int> &ump_count);
    void assign_goods();

   private:
    static Buyer *Instance;                                 // 单例
    vector<BGoods *> m_goods;                               // buyers from file
    vector<Business> m_results;                             // 成交
    unordered_map<string, vector<int>> m_match_intent;      // match
    vector<unordered_map<string, int>> m_gloabl_buy_count;  // 单意向购买数
};

#endif /* !BUYER_H_ */
