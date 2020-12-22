/*
 * @date 2020-03-04
 * @file tools.h
 * @author chier
 */

#ifndef TOOLS_H_
#define TOOLS_H_

#include <chrono>
#include <cmath>
#include <iostream>
#include <random>

class Tools {
   public:
    inline static int RandomInt(const int &l, const int &r);
    inline static double RandomDouble(const double &l, const double &r);
    inline static void ShuffleVector(std::vector<int> &vt);
    inline static std::vector<std::string> Split(std::string str, std::string pattern);
};

inline int Tools::RandomInt(const int &l, const int &r) {
    static std::default_random_engine e(time(nullptr));
    std::uniform_int_distribution<int> u(l, r);
    return u(e);
}

inline double Tools::RandomDouble(const double &l, const double &r) {
    static std::default_random_engine e(time(nullptr));
    std::uniform_real_distribution<double> u(l, r);
    double ans = u(e);
    ans = floor(ans * 1000.000f + 0.5) / 1000.000f;
    return ans;
}
inline void Tools::ShuffleVector(std::vector<int> &vt) {
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(vt.begin(), vt.end(), std::default_random_engine(seed));
}
inline std::vector<std::string> Tools::Split(std::string str, std::string pattern) {
    std::string::size_type pos;
    std::vector<std::string> result;
    str += pattern;  //扩展字符串以方便操作
    int size = str.size();
    for (int i = 0; i < size; i++) {
        pos = str.find(pattern, i);
        if (pos < size) {
            std::string s = str.substr(i, pos - i);
            result.push_back(s);
            i = pos + pattern.size() - 1;
        }
    }
    return result;
}

#endif /* !TOOLS_H_ */
