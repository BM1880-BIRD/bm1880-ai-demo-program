#pragma once

#include <list>
#include <tuple>
#include <iostream>
#include <vector>
#include "pipeline_core.hpp"

template <typename T>
inline std::ostream &operator<<(std::ostream &s, const std::vector<T> &vec) {
    s << "[";
    bool first = true;
    for (auto &ele : vec) {
        if (!first) {
            s << ", ";
        }
        first = false;
        s << ele;
    }
    s << "]";
    return s;
}

template <typename T>
inline std::ostream &operator<<(std::ostream &s, const std::list<T> &list) {
    s << "[";
    bool first = true;
    for (auto &ele : list) {
        if (!first) {
            s << ", ";
        }
        first = false;
        s << ele;
    }
    s << "]";
    return s;
}

namespace pipeline {

template <typename T>
class Dump : public Component<const T &> {
public:
    std::tuple<> process(std::tuple<const T &> tup) {
        std::cerr << std::get<0>(tup) << std::endl;
        return {};
    }
};

}