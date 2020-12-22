/*
 * @date 2020-03-04
 * @file scope_time.h
 * @author chier
 */

#ifndef SCOPE_TIME_H_
#define SCOPE_TIME_H_

#include <chrono>
#include <iostream>

class ScopeTime {
   public:
    ScopeTime() : m_begin(std::chrono::high_resolution_clock::now()) {}
    void LogTime() const {
        auto t =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_begin);
        double elapsed = (double)(t.count() * 1.0) / 1000.0;
        std::cerr << "*运行时间: " << elapsed << "s\n";
    }
    double LogTime() {
        auto t =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_begin);
        double elapsed = (double)(t.count() * 1.0) / 1000.0;
        return elapsed;
    }

   private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_begin;
};

#endif  // SCOPE_TIME_H_