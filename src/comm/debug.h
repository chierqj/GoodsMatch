/*
 * @date 2020-03-04
 * @file debug.h
 * @author chier
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#define DEBUG(...) cerr << "[" << #__VA_ARGS__ << "]:", debug_out(__VA_ARGS__)

#include <iostream>
using std::cerr;
using std::pair;
using std::to_string;

std::string to_string(std::string s);
std::string to_string(const char *s);
std::string to_string(bool b);
void debug_out();

template <typename A, typename B>
std::string to_string(pair<A, B> p) {
    return "(" + to_string(p.first) + ", " + to_string(p.second) + ")";
}
template <typename A>
std::string to_string(A v) {
    bool first = true;
    std::string res = "{";
    for (const auto &x : v) {
        if (!first) {
            res += ", ";
        }
        first = false;
        res += to_string(x);
    }
    res += "}";
    return res;
}
template <typename Head, typename... Tail>
void debug_out(Head H, Tail... T) {
    cerr << " " << to_string(H);
    debug_out(T...);
}

#endif