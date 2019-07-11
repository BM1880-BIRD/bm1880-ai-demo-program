#pragma once

#include <chrono>
#include <cstdio>

class Timer {
public:
    Timer() : name(nullptr), c(0), duration(0) {}
    Timer(const char *name) : name(name), c(0), duration(0) {}
    ~Timer() {
        if (name) {
            print();
        }
    }

    inline void print() {
        fprintf(stderr, "%p %s: count: %lu, total: %ld.%06lds, avg: %.6fs, #op/s: %.6f\n", &name, name != nullptr ? name : "null", c,
                                                                    duration.count() / 1000000, duration.count() % 1000000,
                                                                    duration.count() * 1e-6 / c, 1 / (duration.count() * 1e-6 / c));
    }

    inline void tik() {
        last_tik = std::chrono::high_resolution_clock::now();
    }
    inline void tok() {
        duration += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - last_tik);
        c++;
    }
    inline std::chrono::microseconds total() const {
        return duration;
    }
    inline size_t count() const {
        return c;
    }

private:
    const char *name;
    size_t c;
    std::chrono::high_resolution_clock::time_point last_tik;
    std::chrono::microseconds duration;
};