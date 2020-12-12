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
#include "src/seller/depot.h"
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
    void create_match_intent();
    vector<int> get_intent_order(SGoods *seller, BGoods *buyer);
    void output();
    void contact_result();

    void do_business(Depot::Item &item, BGoods *buyer, SGoods *seller);
    void assign_buyer(BGoods *buyer, bool consider_first_intent);
    void assign_last_buyers();
    vector<queue<BGoods *>> create_buyers(int intent_id, int &buyers_count);
    void assign_goods();
    int get_depot_score(Depot *depot, BGoods *buyer, bool consider_first_intent);

   private:
    static Buyer *Instance;                             // 单例
    vector<BGoods *> m_goods;                           // buyers from file
    vector<Business> m_results;                         // 成交
    unordered_map<string, vector<int>> m_match_intent;  // match
};

#endif /* !BUYER_H_ */
