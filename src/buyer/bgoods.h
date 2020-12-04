/*
** EPITECH PROJECT, 2020
** GoodsMatch
** File description:
** bgoods
*/

#ifndef BGOODS_H_
#define BGOODS_H_

#include <iostream>
#include <unordered_map>
#include <vector>

using namespace std;

struct Intent {
    Intent(const vector<pair<string, string>> intents, vector<int> orders) : values(intents), orders(orders) {}
    void init(const string& breed) {
        map_key = breed;
        sort(values.begin(), values.end());
        for (auto& it : values) map_key += "|" + it.first + "|" + it.second;

        unordered_map<string, vector<int>> breed_scores;
        breed_scores["CF"] = vector<int>{33, 27, 20, 13, 7};
        breed_scores["SR"] = vector<int>{40, 30, 20, 10, 0};

        score = 0;
        for (int i = 0; i < orders.size(); ++i) {
            score += breed_scores[breed][orders[i]];
        }
    }

    vector<pair<string, string>> values;  // 意向值
    vector<int> orders;                   // 意向顺序
    int score;                            // 意向打分
    string map_key;                       // 意向拼接的string，包含breed
};

class BGoods {
   public:
    BGoods();
    BGoods(const string& buyer_id, int hold_time, int buy_count, const string& breed,
           const vector<pair<string, string>>& excepts)
        : m_buyer_id(buyer_id), m_hold_time(hold_time), m_buy_count(buy_count), m_breed(breed), m_intent(excepts) {}
    void debug();
    void create_permutation();

    inline const string& GetBuyerID() const;
    inline const int& GetHoldTime() const;
    inline const int& GetBuyCount() const;
    inline const string& GetBreed() const;
    inline const vector<pair<string, string>>& GetIntents() const;
    inline void SetBuyCount(int buy_count);
    inline const vector<Intent*>& GetPermuIntents() const;
    inline const int& GetUsefulIntent() const;

   private:
    string m_buyer_id;                      // 买家id
    int m_hold_time;                        // 平均持仓时间
    int m_buy_count;                        // 购买数量
    string m_breed;                         // 品种
    vector<pair<string, string>> m_intent;  // 五个意向
    vector<Intent*> m_permu_intents;        // 27种
    int m_useful_intent;                    // 有效意向
};

inline const string& BGoods::GetBuyerID() const { return m_buyer_id; }
inline const int& BGoods::GetHoldTime() const { return m_hold_time; }
inline const int& BGoods::GetBuyCount() const { return m_buy_count; }
inline const string& BGoods::GetBreed() const { return m_breed; }
inline const vector<pair<string, string>>& BGoods::GetIntents() const { return m_intent; }
inline void BGoods::SetBuyCount(int buy_count) { m_buy_count = buy_count; }
inline const vector<Intent*>& BGoods::GetPermuIntents() const { return m_permu_intents; }
inline const int& BGoods::GetUsefulIntent() const { return m_useful_intent; }

#endif /* !BGOODS_H_ */
