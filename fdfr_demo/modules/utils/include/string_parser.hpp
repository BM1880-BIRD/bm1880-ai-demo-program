#pragma once

#include <string>
#include <vector>
#include <sstream>
#include "face_api.hpp"

template <typename T>
T parse_string(const std::string &s);

template <>
inline std::string parse_string<std::string>(const std::string &s) {
    return s;
}

template <>
inline double parse_string<double>(const std::string &s) {
    std::stringstream ss;
    double f = 0;
    ss << s;
    ss >> f;
    return f;
}

template <>
inline float parse_string<float>(const std::string &s) {
    std::stringstream ss;
    float f = 0;
    ss << s;
    ss >> f;
    return f;
}

template <>
inline int parse_string<int>(const std::string &s) {
    std::stringstream ss;
    int i = 0;
    ss << s;
    ss >> i;
    return i;
}

template <>
inline bool parse_string<bool>(const std::string &s) {
    return (s == "true");
}

template <>
inline size_t parse_string<size_t>(const std::string &s) {
    std::stringstream ss;
    size_t i = 0;
    ss << s;
    ss >> i;
    return i;
}

template <>
inline std::vector<std::string> parse_string<std::vector<std::string>>(const std::string &s) {
    std::vector<std::string> v;
    size_t offset = 0;
    size_t index;
    while ((index = s.find('|', offset)) != std::string::npos) {
        v.emplace_back(s.begin() + offset, s.begin() + index);
        offset = index + 1;
    }
    if (offset < s.length()) {
        v.emplace_back(s.begin() + offset, s.end());
    }
    return v;
}

template <>
inline face_algorithm_t parse_string<face_algorithm_t>(const std::string &s) {
    if (s == "tinyssh") {
        return FD_TINYSSH;
    } else if (s == "ssh") {
        return FD_SSH;
    } else if (s == "mtcnn") {
        return FD_MTCNN;
    } else if (s == "mtcnn_native") {
        return FD_MTCNN_NATIVE;
    } else if (s == "bmface") {
        return FR_BMFACE;
    } else {
        return INVALID_ALOG;
    }
}
