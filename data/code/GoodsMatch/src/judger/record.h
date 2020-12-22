#ifndef RECORD_ROW_H_
#define RECORD_ROW_H_

#include <string>
using namespace std;

class Record {
   public:
    Record();
    Record(const string &buyer_id, const string &seller_id, const string &breed, const string &good_id,
           const string &depot_id, int stock, const string &intent)
        : m_buyer_id(buyer_id),
          m_seller_id(seller_id),
          m_breed(breed),
          m_good_id(good_id),
          m_depot_id(depot_id),
          m_good_stock(stock),
          m_intent(intent) {}
    ~Record();
    void debug();

    inline const string &GetBuyerId() const { return m_buyer_id; }
    inline const string &GetSellerID() const { return m_seller_id; }
    inline const string &GetBreed() const { return m_breed; }
    inline const string &GetGoodID() const { return m_good_id; }
    inline const string &GetDepotID() const { return m_depot_id; }
    inline const int &GetGoodStock() const { return m_good_stock; }
    inline const string &GetIntent() const { return m_intent; }

   private:
    string m_buyer_id;
    string m_seller_id;
    string m_breed;
    string m_good_id;
    string m_depot_id;
    int m_good_stock;
    string m_intent;
};

#endif