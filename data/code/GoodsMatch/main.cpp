#include <iostream>

#include "src/comm/config.h"
#include "src/comm/log.h"
#include "src/simulater/simulation.h"

void run() {
    auto sim = new Simulation();
    sim->RunFrameWork();
}

int main(int argc, char **argv) {
    Config::Initialize(argc, argv);
    FILE *fp = fopen(Config::g_conf["log_file"].c_str(), "w");
    log_set_fp(fp);

    run();

    fclose(fp);
    return 0;
}
