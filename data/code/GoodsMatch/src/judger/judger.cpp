#include "judger.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
using namespace std;

#include "src/comm/config.h"
#include "src/comm/log.h"
#include "src/comm/scope_time.h"
#include "src/comm/tools.h"

void Judger::Execute() {
    this->read_data();

    this->CheckStock();
    this->CheckExist();
    this->CheckIntent();

    auto score = GetScore();
    log_info("* 得分: %f ", score);
}

void Judger::CheckStock() {
    log_info(" CheckStock...");

    //货物总数量检查
    unordered_map<string, int> hashMap;
    for (auto &item : m_sgoods) {
        hashMap[item->GetGoodID()] += item->GetGoodStock();
    }
    for (auto &item : m_records) {
        hashMap[item->GetGoodID()] -= item->GetGoodStock();
    }
    for (auto &item : hashMap) {
        if (item.second != 0) {
            log_error(" 货物%s总数量不匹配 ", item.first.c_str());
            exit(1);
        }
    }

    //卖方货物数量检查
    hashMap.clear();
    for (auto &item : m_sgoods) {
        hashMap[item->GetSellerID() + item->GetGoodID()] += item->GetGoodStock();
    }
    for (auto &item : m_records) {
        auto &good_stock = hashMap[item->GetSellerID() + item->GetGoodID()];
        good_stock -= item->GetGoodStock();
    }
    for (auto &item : hashMap) {
        if (item.second != 0) {
            log_error(" 卖方货物%s总数量不匹配 ", item.first.c_str());
            exit(1);
        }
    }

    //卖方仓库数量检查
    hashMap.clear();
    for (auto &item : m_sgoods) {
        hashMap[item->GetSellerID() + item->GetBreed() + item->GetDepotID()] += item->GetGoodStock();
    }
    for (auto &item : m_records) {
        auto &good_stock = hashMap[item->GetSellerID() + item->GetBreed() + item->GetDepotID()];
        good_stock -= item->GetGoodStock();
    }
    for (auto &item : hashMap) {
        if (item.second != 0) {
            log_error(" 卖方品种仓库%s总数量不匹配 ", item.first.c_str());
            exit(1);
        }
    }

    log_info(" CheckStock done.");
}

void Judger::CheckExist() {
    log_info(" CheckExist...");

    //检查买方id存在
    unordered_set<string> hashSet;
    for (auto &item : m_bgoods) {
        hashSet.insert(item->GetBuyerID());
    }
    for (auto &item : m_records) {
        if (hashSet.find(item->GetBuyerId()) == hashSet.end()) {
            log_error(" buyer_id %s is not exist.", item->GetBuyerId().c_str());
            exit(1);
        }
    }

    //检查卖方id存在
    hashSet.clear();
    for (auto &item : m_sgoods) {
        hashSet.insert(item->GetSellerID());
    }
    for (auto &item : m_records) {
        if (hashSet.find(item->GetSellerID()) == hashSet.end()) {
            log_error(" seller_id %s is not exist.", item->GetSellerID().c_str());
            exit(1);
        }
    }

    //检查品种存在
    hashSet.clear();
    for (auto &item : m_sgoods) {
        hashSet.insert(item->GetBreed());
    }
    for (auto &item : m_records) {
        if (hashSet.find(item->GetBreed()) == hashSet.end()) {
            log_error(" breed %s is not exist.", item->GetBreed().c_str());
            exit(1);
        }
    }

    //检查货物id存在
    hashSet.clear();
    for (auto &item : m_sgoods) {
        hashSet.insert(item->GetGoodID());
    }
    for (auto &item : m_records) {
        if (hashSet.find(item->GetGoodID()) == hashSet.end()) {
            log_error(" good_id %s is not exist.", item->GetGoodID().c_str());
            exit(1);
        }
    }

    //检查仓库存在
    hashSet.clear();
    for (auto &item : m_sgoods) {
        hashSet.insert(item->GetDepotID());
    }
    for (auto &item : m_records) {
        if (hashSet.find(item->GetDepotID()) == hashSet.end()) {
            log_error(" good_id %s is not exist.", item->GetDepotID().c_str());
            exit(1);
        }
    }

    log_info(" CheckExist done.");
}

void Judger::CheckIntent() {
    log_info(" CheckIntent...");

    //意向合法检查
    {
        unordered_map<string, int> buyer_intent_nums;
        for (auto &item : m_bgoods) {
            int num = 0;
            for (auto &elem : item->GetIntents()) {
                if (elem.first != "") num++;
            }
            buyer_intent_nums[item->GetBuyerID() + item->GetBreed()] = num;
        }
        int index = 0;
        for (auto &item : m_records) {
            auto intent = item->GetIntent();
            auto nums = Tools::Split(intent, "-");
            int maxNum = buyer_intent_nums[item->GetBuyerId() + item->GetBreed()];
            for (auto &elem : nums) {
                if (atoi(elem.c_str()) > maxNum) {
                    log_error("第%d个答案对应意向顺序%s不合法", index, intent.c_str());
                    exit(0);
                }
            }
            index++;
        }
    }

    //第一意向检查
    {
        unordered_set<string> first_intent_records;  // k: 买方客户id+品种
        for (auto &item : m_records) {
            if (item->GetIntent()[0] == '1') {
                first_intent_records.insert(item->GetBuyerId() + item->GetBreed());
            }
        }

        unordered_map<string, vector<BGoods *>> intents;  // k: 品种+第一意向值
        for (auto &item : m_bgoods) {
            if (item->GetIntents()[0].first.size() < 1) continue;
            auto first_intent = item->GetBreed() + item->GetIntents()[0].first + item->GetIntents()[0].second;
            intents[first_intent].push_back(item);
        }
        for (auto &item : intents) {
            auto &vt = item.second;
            sort(vt.begin(), vt.end(),
                 [&](const BGoods *g1, const BGoods *g2) { return g1->GetHoldTime() > g2->GetHoldTime(); });
            int index = 0;
            for (; index < vt.size(); ++index) {
                if (first_intent_records.find(vt[index]->GetBuyerID() + vt[index]->GetBreed()) ==
                    first_intent_records.end())
                    break;
            }
            if (index >= vt.size()) continue;
            auto flag = vt[index];
            for (; index < vt.size(); ++index) {
                if (vt[index]->GetHoldTime() < flag->GetHoldTime() &&
                    first_intent_records.find(vt[index]->GetBuyerID() + vt[index]->GetBreed()) !=
                        first_intent_records.end()) {
                    log_error(" 第一意向违规 ");
                    exit(1);
                }
            }
        }
    }

    log_info(" CheckIntent done.");
}

void Judger::ReadBuyerData() {
    ScopeTime t;

    auto file_path = Config::g_conf["buyer_file"];
    ifstream fin(file_path);
    string line;
    bool fir = true;
    int count = 0;
    while (fin >> line) {
        if (fir) {
            fir = false;
            continue;
        }
        auto row_data = Tools::Split(line, ",");
        assert(row_data.size() == 14);

        vector<pair<string, string>> excepts{{row_data[4], row_data[5]},
                                             {row_data[6], row_data[7]},
                                             {row_data[8], row_data[9]},
                                             {row_data[10], row_data[11]},
                                             {row_data[12], row_data[13]}};

        auto good = new BGoods(row_data[0], atoi(row_data[1].c_str()), atoi(row_data[2].c_str()), row_data[3], excepts);
        m_bgoods.push_back(good);
    }

    log_info("[load %s] [line: %d] [%.3fs]", file_path.c_str(), m_bgoods.size(), t.LogTime());
}

void Judger::ReadSellerData() {
    ScopeTime t;

    auto file_path = Config::g_conf["seller_file"];
    ifstream fin(file_path);
    string line;
    bool fir = true;
    int count = 0;
    while (fin >> line) {
        if (fir) {
            fir = false;
            continue;
        }
        auto row_data = Tools::Split(line, ",");
        assert(row_data.size() == 10);
        auto good = new SGoods(row_data[0], row_data[1], row_data[2], atoi(row_data[3].c_str()), row_data[4],
                               row_data[5], row_data[6], row_data[7], row_data[8], row_data[9]);
        m_sgoods.push_back(good);
    }

    log_info("[load %s] [line: %d] [%.3fs]", file_path.c_str(), m_sgoods.size(), t.LogTime());
}

void Judger::ReadRecordData() {
    ScopeTime t;

    auto file_path = Config::g_conf["result_file"];
    ifstream fin(file_path);
    string line;
    bool fir = true;
    int count = 0;
    while (fin >> line) {
        if (fir) {
            fir = false;
            continue;
        }
        auto row_data = Tools::Split(line, ",");
        assert(row_data.size() == 7);
        auto record = new Record(row_data[0], row_data[1], row_data[2], row_data[3], row_data[4],
                                 atoi(row_data[5].c_str()), row_data[6]);
        m_records.push_back(record);
    }

    log_info("[load %s] [line: %d] [%.3fs]", file_path.c_str(), m_records.size(), t.LogTime());
}

void Judger::read_data() {
    ReadBuyerData();
    ReadSellerData();
    ReadRecordData();
}

double Judger::GetScore() {
    unordered_map<string, int> buyerBreed_stock;  //客户在某个品种买入的货物数量
    for (auto &item : m_records) {
        buyerBreed_stock[item->GetBuyerId() + item->GetBreed()] += item->GetGoodStock();
    }

    unordered_map<string, double>
        buyerBreed_score;  //客户在某个品种的满意率评分  满意率评分由hope_score和dairy_score构成
    //统计hope_score
    for (auto &item : m_records) {
        int allNum = buyerBreed_stock[item->GetBuyerId() + item->GetBreed()];
        int curNum = item->GetGoodStock();
        auto intents = item->GetIntent();
        auto intentID = Tools::Split(intents, "-");

        int score = 0;
        if (item->GetBreed() == "CF") {
            for (auto &elem : intentID) {
                score += cf_score[atoi(elem.c_str())];
            }
        } else if (item->GetBreed() == "SR") {
            for (auto &elem : intentID) {
                score += sr_score[atoi(elem.c_str())];
            }
        } else {
            log_error(" %s品种不存在 ", item->GetBreed().c_str());
        }
        if (score > 100) {
            log_error(" 意向分超过100 ");
            item->debug();
            exit(0);
        }
        buyerBreed_score[item->GetBuyerId() + item->GetBreed()] += ((double)curNum / allNum) * score;
    }
    unordered_map<string, double> buyerBreed_hopeScore;

    auto file_path = Config::g_conf["score_file"];
    ofstream fout(file_path);
    fout << "买方客户,得分\n";
    for (auto &item : buyerBreed_score) {
        item.second = item.second * hopeScore_weight;
        buyerBreed_hopeScore[item.first] = item.second;
        fout << item.first << "," << item.second << "\n";
    }
    fout.close();

    //统计diary_score
    unordered_map<string, unordered_set<string>> buyerBreed_depotID;
    for (auto &item : m_records) {
        buyerBreed_depotID[item->GetBuyerId() + item->GetBreed()].insert(item->GetDepotID());
    }
    unordered_map<string, double> buyerBreed_diaryScore;
    for (auto &item : buyerBreed_depotID) {
        double diary_score = 100;
        int depotCounts = item.second.size();
        if (item.first.back() == 'F')
            diary_score = diary_score - ((double)100 / cf_property_num) * (depotCounts - 1);
        else if (item.first.back() == 'R')
            diary_score = diary_score - ((double)100 / sr_property_num) * (depotCounts - 1);
        else
            log_error(" 品种%s不存在 ", item.first.c_str());
        if (diary_score > 100) {
            log_error(" 记录分超过100 ");
            exit(0);
        }
        diary_score = diary_score * diaryScore_weight;
        buyerBreed_diaryScore[item.first] = diary_score;
        buyerBreed_score[item.first] += diary_score;
    }

    //统计两个品种货物总数
    int cf_allStocks = 0, sr_allStocks = 0;
    for (auto &item : m_records) {
        if (item->GetBreed() == "CF") {
            cf_allStocks += item->GetGoodStock();
        } else if (item->GetBreed() == "SR") {
            sr_allStocks += item->GetGoodStock();
        } else {
            log_error(" %s品种不存在 ", item->GetBreed().c_str());
        }
    }

    //计算两个品种的总分
    double cf_allScore = 0, sr_allScore = 0;
    double cf_hopeScore = 0, sr_hopeScore = 0, cf_diaryScore = 0, sr_diaryScore = 0;
    for (auto &item : buyerBreed_score) {
        auto buyerId_breed = item.first;
        auto customer_score = item.second;
        if (buyerId_breed.back() == 'F') {
            double score = ((double)buyerBreed_stock[buyerId_breed] / cf_allStocks) * customer_score;
            cf_hopeScore +=
                ((double)buyerBreed_stock[buyerId_breed] / cf_allStocks) * buyerBreed_hopeScore[buyerId_breed];
            cf_diaryScore +=
                ((double)buyerBreed_stock[buyerId_breed] / cf_allStocks) * buyerBreed_diaryScore[buyerId_breed];
            cf_allScore += score;
        } else if (buyerId_breed.back() == 'R') {
            double score = ((double)buyerBreed_stock[buyerId_breed] / sr_allStocks) * customer_score;
            sr_hopeScore +=
                ((double)buyerBreed_stock[buyerId_breed] / sr_allStocks) * buyerBreed_hopeScore[buyerId_breed];
            sr_diaryScore +=
                ((double)buyerBreed_stock[buyerId_breed] / sr_allStocks) * buyerBreed_diaryScore[buyerId_breed];
            sr_allScore += score;
        } else {
            log_error(" %s品种不存在 ", buyerId_breed.c_str());
        }
    }

    map<int, int> depotCount_buyerCount;
    for (auto &item : buyerBreed_depotID) {
        int depotCount = item.second.size();
        depotCount_buyerCount[depotCount]++;
    }
    for (auto &item : depotCount_buyerCount) {
        log_debug("* 仓库个数: %d, 客户数: %d ", item.first, item.second);
    }

    log_debug("* [CF] --- [hope_score: %f, diary_score: %f, tol: %f]", cf_hopeScore, cf_diaryScore,
              cf_hopeScore + cf_diaryScore);
    log_debug("* [SR] --- [hope_score: %f, diary_score: %f, tol: %f]", sr_hopeScore, sr_diaryScore,
              sr_hopeScore + sr_diaryScore);
    log_debug("* [ALL]--- [hope_score: %f, diary_score: %f]", cf_hopeScore + sr_hopeScore,
              cf_diaryScore + sr_diaryScore);

    return cf_allScore + sr_allScore;
}