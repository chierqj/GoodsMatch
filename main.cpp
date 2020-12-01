#include <iostream>

#include "src/comm/config.h"
#include "src/comm/log.h"
#include "src/comm/scope_time.h"
#include "src/simulater/simulation.h"

void run() {
    ScopeTime t;
    auto sim = new Simulation();
    sim->RunFrameWork();
    log_info("[总运行时间: %.3fs]", t.LogTime());
}

int main(int argc, char **argv) {
    Config::Initialize(argc, argv);
    FILE *fp = fopen(Config::g_conf["log_file"].c_str(), "w");
    log_set_fp(fp);

    run();

    fclose(fp);
    return 0;
}
