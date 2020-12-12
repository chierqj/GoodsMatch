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

#include "depot.h"
#include "sgoods.h"
#include "src/buyer/bgoods.h"
using namespace std;

struct Propoty {
    Propoty(vector<pair<string, string>> values, const string& map_key) : values(values), map_key(map_key) {}
    Propoty(const string& map_key) : map_key(map_key) {}
    vector<pair<string, string>> values;
    string map_key;
};

class Seller {
   public:
    static Seller* GetInstance();

   public:
    void Execute();
    void debug();
    inline vector<Depot*>& GetDepots();
    inline unordered_map<string, int>& GetGloalCount();
    inline unordered_map<string, vector<Propoty*>>& GetPropotys();

   private:
    Seller() {}
    Seller(const Seller&) = delete;
    Seller& operator=(const Seller&) = delete;

    void read_data();
    void create_hashmap();
    void create_permutation(SGoods* seller);

   private:
    static Seller* Instance;                            // 单例
    vector<SGoods*> m_goods;                            // 总货物
    vector<Depot*> m_depots;                            // 全部的seller
    unordered_map<string, vector<Propoty*>> m_propoty;  // good_id -> 属性组合
    unordered_map<string, int> m_global_count;          // 单属性[breed+value]
};
inline vector<Depot*>& Seller::GetDepots() { return m_depots; }
inline unordered_map<string, int>& Seller::GetGloalCount() { return m_global_count; }
inline unordered_map<string, vector<Propoty*>>& Seller::GetPropotys() { return m_propoty; }

#endif /* !SELLER_H_ */
