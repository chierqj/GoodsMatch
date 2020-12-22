/*
 * @date 2020-03-04
 * @file config.cc
 * @author chier
 */

#include "config.h"

#include <fstream>
#include <iostream>
#include <cassert>

#include "debug.h"
#include "log.h"

std::unordered_map<std::string, std::string> Config::g_conf;

void Config::Initialize(int argc, char **argv) {
    assert(argc >= 2);
    std::string file = argv[1];
    std::ifstream fin(file);
    std::string line;
    while (fin >> line) {
        int p = line.find("=");
        std::string key = line.substr(0, p);
        std::string value = line.substr(p + 1, line.size() - p - 1);
        g_conf.insert({key, value});
    }
    for (auto &it : g_conf) {
        log_info("config [%s = %s]", it.first.c_str(), it.second.c_str());
    }
}
