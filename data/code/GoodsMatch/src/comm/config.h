/*
 * @date 2020-03-04
 * @file config.h
 * @author chier
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <unordered_map>

class Config {
   public:
    static void Initialize(int argc, char **argv);
    static std::unordered_map<std::string, std::string> g_conf;
};

#endif /* !CONFIG_H_ */
