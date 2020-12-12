/*
** EPITECH PROJECT, 2020
** GoodsMatch
** File description:
** seller-row
*/

#ifndef SELLER_ROW_H_
#define SELLER_ROW_H_

#include <string>
#include <vector>
using namespace std;

class SGoods {
   public:
    SGoods();
    SGoods(const string &seller_id, const string &breed, const string &good_id, int good_stock, const string &depot_id,
           const string &brand, const string &place, const string &year, const string &level, const string &category)
        : m_seller_id(seller_id),
          m_breed(breed),
          m_good_id(good_id),
          m_good_stock(good_stock),
          m_depot_id(depot_id),
          m_brand(brand),
          m_place(place),
          m_year(year),
          m_level(level),
          m_category(category) {}
    void debug();

    inline const string &GetSellerID() const;
    inline const string &GetBreed() const;
    inline const string &GetGoodID() const;
    inline const int &GetGoodStock() const;
    inline const string &GetDepotID() const;
    inline const string &GetBrand() const;
    inline const string &GetPlace() const;
    inline const string &GetYear() const;
    inline const string &GetLevel() const;
    inline const string &GetCategory() const;
    inline void SetGoodStock(int good_stock);
    inline const vector<string> &GetSinglePropoty() const;
    inline const int &GetGloablBuyCount() const;
    inline void SetGloablBuyCount(int count);

   private:
    string m_seller_id;               // 卖家id
    string m_breed;                   // 品种
    string m_good_id;                 // 货物id
    int m_good_stock;                 // 货物数量, 库存
    string m_depot_id;                // 仓库id [hash]
    string m_brand;                   // 品牌 [hash]
    string m_place;                   // 产地 [hash]
    string m_year;                    // 年度 [hash]
    string m_level;                   // 等级 [hash]
    string m_category;                // 类别 [hash]
    vector<string> m_single_propoty;  // 单属性组合

    int m_gloabl_buy_count;
};

inline const string &SGoods::GetSellerID() const { return m_seller_id; }
inline const string &SGoods::GetBreed() const { return m_breed; }
inline const string &SGoods::GetGoodID() const { return m_good_id; }
inline const int &SGoods::GetGoodStock() const { return m_good_stock; }
inline const string &SGoods::GetDepotID() const { return m_depot_id; }
inline const string &SGoods::GetBrand() const { return m_brand; }
inline const string &SGoods::GetPlace() const { return m_place; }
inline const string &SGoods::GetYear() const { return m_year; }
inline const string &SGoods::GetLevel() const { return m_level; }
inline const string &SGoods::GetCategory() const { return m_category; }
inline void SGoods::SetGoodStock(int good_stock) { m_good_stock = good_stock; }
inline const vector<string> &SGoods::GetSinglePropoty() const { return m_single_propoty; }
inline const int &SGoods::GetGloablBuyCount() const { return m_gloabl_buy_count; }
inline void SGoods::SetGloablBuyCount(int count) { m_gloabl_buy_count = count; }

#endif /* !SELLER_ROW_H_ */
