#include "simulation.h"

#include "src/buyer/buyer.h"
#include "src/comm/config.h"
#include "src/judger/judger.h"
#include "src/seller/seller.h"

void Simulation::RunFrameWork() {
    auto only_judge = Config::g_conf["only_judge"];
    if (only_judge == "true") {
        auto judger = new Judger();
        judger->Execute();
        return;
    }

    auto seller = Seller::GetInstance();
    seller->Execute();
    auto buyer = Buyer::GetInstance();
    buyer->Execute();
    auto judger = new Judger();
    judger->Execute();
}