/*
** EPITECH PROJECT, 2020
** GoodsMatch
** File description:
** bgoods
*/

#ifndef BGOODS_H_
#define BGOODS_H_

#include <iostream>
#include <vector>
using namespace std;

class BGoods {
   public:
    BGoods();
    BGoods(const string& buyer_id, int hold_time, int buy_count, const string& breed,
           const vector<pair<string, string>>& excepts)
        : m_buyer_id(buyer_id), m_hold_time(hold_time), m_buy_count(buy_count), m_breed(breed), m_excepts(excepts) {}
    void debug();

    inline const string& GetBuyerID() const;
    inline const int& GetHoldTime() const;
    inline const int& GetBuyCount() const;
    inline const string& GetBreed() const;
    inline const vector<pair<string, string>>& GetExcepts() const;
    inline void SetBuyCount(int buy_count);
    void GetPermutation(vector<vector<pair<string, string>>>& res_expects, vector<vector<int>>& expect_order);

   private:
    string m_buyer_id;                       // 买家id
    int m_hold_time;                         // 平均持仓时间
    int m_buy_count;                         // 购买数量
    string m_breed;                          // 品种
    vector<pair<string, string>> m_excepts;  // 五个意向
};

inline const string& BGoods::GetBuyerID() const { return m_buyer_id; }
inline const int& BGoods::GetHoldTime() const { return m_hold_time; }
inline const int& BGoods::GetBuyCount() const { return m_buy_count; }
inline const string& BGoods::GetBreed() const { return m_breed; }
inline const vector<pair<string, string>>& BGoods::GetExcepts() const { return m_excepts; }
inline void BGoods::SetBuyCount(int buy_count) { m_buy_count = buy_count; }

#endif /* !BGOODS_H_ */
