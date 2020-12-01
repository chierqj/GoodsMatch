/*
** EPITECH PROJECT, 2020
** GoodsMatch
** File description:
** seller
*/

#ifndef SELLER_H_
#define SELLER_H_

#include <deque>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "sgoods.h"
#include "src/buyer/bgoods.h"
using namespace std;

typedef unordered_map<string, deque<SGoods*>> HashMap;
typedef unordered_map<string, deque<SGoods*>>::iterator HashMapItr;

class Seller {
   public:
    static Seller* GetInstance();

   public:
    void Execute();
    int GetGoods(HashMapItr& result, const pair<string, string>& expect, const string& breed);
    void EraseGoods(SGoods* goods);
    void debug();

    inline unordered_set<SGoods*>& GetLeftGoods();

   private:
    Seller() {}
    Seller(const Seller&) = delete;
    Seller& operator=(const Seller&) = delete;

    void read_data();
    void sort_goods();
    void create_hashmap();

   private:
    static Seller* Instance;                   // 单例
    vector<SGoods*> m_goods;                   // 总货物
    unordered_set<SGoods*> m_left_goods;       // 剩余货物
    unordered_map<string, HashMap> m_hashmap;  // 获取hash映射
};

inline unordered_set<SGoods*>& Seller::GetLeftGoods() { return m_left_goods; }

#endif /* !SELLER_H_ */
