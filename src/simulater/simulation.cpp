#include "simulation.h"

#include "src/buyer/buyer.h"
#include "src/judger/judger.h"
#include "src/seller/seller.h"

void Simulation::RunFrameWork() {
    auto seller = Seller::GetInstance();
    seller->Execute();
    auto buyer = new Buyer();
    buyer->Execute();
    auto judger = new Judger();
    judger->Execute();
}