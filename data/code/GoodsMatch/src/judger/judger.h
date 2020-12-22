#ifndef JUDGER_H_
#define JUDGER_H_

#include <vector>
using namespace std;

#include "src/buyer/bgoods.h"
#include "src/judger/record.h"
#include "src/seller/sgoods.h"

class Judger {
   public:
    int cf_score[6] = {0, 33, 27, 20, 13, 7};
    int sr_score[6] = {0, 40, 30, 20, 10, 0};
    const double hopeScore_weight = 0.6;
    const double diaryScore_weight = 0.4;
    const int cf_property_num = 5;
    const int sr_property_num = 4;

    void Execute();

   private:
    void CheckStock();
    void CheckExist();
    void CheckIntent();

    void ReadBuyerData();
    void ReadSellerData();
    void ReadRecordData();
    void read_data();

    double GetScore();

    vector<SGoods*> m_sgoods;
    vector<BGoods*> m_bgoods;
    vector<Record*> m_records;
};

#endif