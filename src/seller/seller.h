/*
** EPITECH PROJECT, 2020
** GoodsMatch
** File description:
** seller
*/

#ifndef SELLER_H_
#define SELLER_H_

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

struct Depot {
    Depot(const string& depot_id) : depot_id(depot_id) {}
    void decrease_stock(const SGoods* seller, int val) {
        for (auto& propoty : seller->GetPropoty()) {
            string key = seller->GetBreed() + "|" + propoty->map_key;
            sellers_tol_stock[key] -= val;
            assert(sellers_tol_stock[key] >= 0);
        }
    }

    void decrease_stock(const SGoods* seller, int val, unordered_map<string, int>& ump) {
        for (auto& propoty : seller->GetPropoty()) {
            string key = seller->GetBreed() + "|" + propoty->map_key;
            sellers_tol_stock[key] -= val;
            ump[key] -= val;
        }
    }

    void pop_front(const string& breed, const string& expect) {
        while (!sellers_groupby_except[expect].empty() && sellers_groupby_except[expect].front()->GetGoodStock() <= 0) {
            useful_sellers[breed].erase(sellers_groupby_except[expect].front());
            sellers_groupby_except[expect].pop_front();
        }
    }

    string depot_id;
    unordered_map<string, deque<SGoods*>> sellers_groupby_except;  // key = [breed + name + val]
    unordered_map<string, int> sellers_tol_stock;                  // key = [breed + name + val]
    unordered_map<string, unordered_set<SGoods*>> useful_sellers;  // 可用的sellers [breed]
};

class Seller {
   public:
    static Seller* GetInstance();

   public:
    void Execute();
    void debug();

    inline unordered_map<string, Depot*>& GetSellers();
    inline unordered_map<string, int>& GetGlobalCount();

   private:
    Seller() {}
    Seller(const Seller&) = delete;
    Seller& operator=(const Seller&) = delete;

    void read_data();
    void sort_goods();
    void create_hashmap();

   private:
    static Seller* Instance;                                // 单例
    vector<SGoods*> m_goods;                                // 总货物
    unordered_map<string, Depot*> m_sellers_groupby_depot;  // depot_id -> depot;
    unordered_map<string, int> m_global_count;              // 全局计数器,单属性
};

inline unordered_map<string, Depot*>& Seller::GetSellers() { return m_sellers_groupby_depot; }

inline unordered_map<string, int>& Seller::GetGlobalCount() { return m_global_count; }

#endif /* !SELLER_H_ */
