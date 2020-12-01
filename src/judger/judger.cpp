#include "judger.h"

#include <algorithm>
#include <cassert>
#include <fstream>
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
    log_info("[Score: %.3f]", score);
}

void Judger::CheckStock() {
    log_info("* CheckStock...");

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
            log_error("* 货物%s总数量不匹配 ", item.first.c_str());
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
            log_error("* 卖方货物%s总数量不匹配 ", item.first.c_str());
            exit(1);
        }
    }

    log_info("* CheckStock done.");
}

void Judger::CheckExist() {
    log_info("* CheckExist...");

    //检查买方id存在
    unordered_set<string> hashSet;
    for (auto &item : m_bgoods) {
        hashSet.insert(item->GetBuyerID());
    }
    for (auto &item : m_records) {
        if (hashSet.find(item->GetBuyerId()) == hashSet.end()) {
            log_error("* buyer_id %s is not exist.", item->GetBuyerId().c_str());
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
            log_error("* seller_id %s is not exist.", item->GetSellerID().c_str());
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
            log_error("* breed %s is not exist.", item->GetBreed().c_str());
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
            log_error("* good_id %s is not exist.", item->GetGoodID().c_str());
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
            log_error("* good_id %s is not exist.", item->GetDepotID().c_str());
            exit(1);
        }
    }

    log_info("* CheckExist done.");
}

void Judger::CheckIntent() {
    log_info("* CheckIntent...");

    //意向合法检查
    {
        unordered_map<string, int> buyer_intent_nums;
        for (auto &item : m_bgoods) {
            int num = 0;
            for (auto &elem : item->GetExcepts()) {
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
                    log_error("* 第%d个答案对应意向顺序%s不合法", index, intent.c_str());
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
            if (item->GetExcepts()[0].first.size() < 1) continue;
            auto first_intent = item->GetBreed() + item->GetExcepts()[0].first + item->GetExcepts()[0].second;
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
                    log_error("* 第一意向违规 ");
                    exit(1);
                }
            }
        }
    }

    log_info("* CheckIntent done.");
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
    unordered_map<string, int> buyer_good_nums;
    for (auto &item : m_records) {
        buyer_good_nums[item->GetBuyerId() + item->GetBreed()] += item->GetGoodStock();
    }

    unordered_map<string, double> buyer_score;
    for (auto &item : m_records) {
        int allNum = buyer_good_nums[item->GetBuyerId() + item->GetBreed()];
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
        buyer_score[item->GetBuyerId() + item->GetBreed()] += ((double)curNum / allNum) * score;
    }
    for (auto &item : buyer_score) {
        item.second = item.second * hopeScore_weight;
    }

    unordered_map<string, unordered_set<string>> buyerBreed_depotID;
    for (auto &item : m_records) {
        buyerBreed_depotID[item->GetBuyerId() + item->GetBreed()].insert(item->GetDepotID());
    }
    for (auto &item : buyerBreed_depotID) {
        double diary_score = 100;
        int depotCounts = item.second.size();
        diary_score = diary_score - ((double)100 / property_num) * (depotCounts - 1);
        diary_score = diary_score * diaryScore_weight;
        buyer_score[item.first] += diary_score;
    }

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

    double cf_allScore = 0, sr_allScore;
    for (auto &item : buyer_score) {
        auto buyerId_breed = item.first;
        auto customer_score = item.second;
        if (buyerId_breed.back() == 'F') {
            cf_allStocks += ((double)buyer_good_nums[buyerId_breed] / cf_allStocks) * customer_score;
        } else if (buyerId_breed.back() == 'R') {
            sr_allStocks += ((double)buyer_good_nums[buyerId_breed] / sr_allStocks) * customer_score;
        } else {
            log_error(" %s品种不存在 ", buyerId_breed.c_str());
        }
    }

    return cf_allStocks + sr_allStocks;
}