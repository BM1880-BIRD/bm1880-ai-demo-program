#pragma once

#include <string>
#include <vector>
#include <type_traits>

namespace base64 {

template<typename Iter>
std::string encode(Iter begin, Iter end) {
    static_assert(std::is_same<typename std::iterator_traits<Iter>::iterator_category, std::random_access_iterator_tag>::value, "");
    static_assert(sizeof(*begin) == 1, "");
    static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                "abcdefghijklmnopqrstuvwxyz"
                                "0123456789+/";
    std::string result;
    size_t size = std::distance(begin, end);
    for (size_t i = 0; i + 2 < size; i += 3) {
        uint32_t x = (*(begin + i) << 16) + (*(begin + i + 1) << 8) + *(begin + i + 2);
        result += table[(x >> 18) & 63];
        result += table[(x >> 12) & 63];
        result += table[(x >> 6) & 63];
        result += table[x & 63];
    }
    if (size % 3 == 1) {
        uint32_t x = (*(begin + size - 1) << 16);
        result += table[(x >> 18) & 63];
        result += table[(x >> 12) & 63];
        result += "==";
    } else if (size % 3 == 2) {
        uint32_t x = (*(begin + size - 2) << 16) + (*(begin + size - 1) << 8);
        result += table[(x >> 18) & 63];
        result += table[(x >> 12) & 63];
        result += table[(x >> 6) & 63];
        result += "=";
    }
    return result;
}

template<typename Iter>
std::vector<unsigned char> decode(Iter begin, Iter end) {
    static_assert(std::is_same<typename std::iterator_traits<Iter>::iterator_category, std::random_access_iterator_tag>::value, "");
    static_assert(sizeof(*begin) == 1, "");
    size_t size = std::distance(begin, end);
    auto trans = [](char c) -> char {
        if (c >= 'A' && c <= 'Z') {
            return c - 'A';
        } else if (c >= 'a' && c <= 'z') {
            return c - 'a' + ('Z' - 'A' + 1);
        } else if (c >= '0' && c <= '9') {
            return c - '0' + ('Z' - 'A' + 1) + ('z' - 'a' + 1);
        } else if (c == '+') {
            return ('9' - '0' + 1) + ('Z' - 'A' + 1) + ('z' - 'a' + 1);
        } else if (c == '/') {
            return 1 + ('9' - '0' + 1) + ('Z' - 'A' + 1) + ('z' - 'a' + 1);
        } else if (c == '=') {
            return 0;
        }
    };

    std::vector<unsigned char> result;
    result.reserve(size * 3 / 4);
    for (size_t i = 0; i < size; i += 4) {
        uint32_t x = (trans(*(begin + i)) << 18) + (trans(*(begin + i + 1)) << 12) + (trans(*(begin + i + 2)) << 6) + trans(*(begin + i + 3));
        result.push_back(x >> 16);
        if (*(begin + i + 2) != '=') {
            result.push_back((x >> 8) & 0xff);
        }
        if (*(begin + i + 3) != '=') {
            result.push_back(x & 0xff);
        }
    }
    return result;
}

template <typename Container>
std::string encode(const Container &c) {
    return encode(c.begin(), c.end());
}

template <typename Container>
std::vector<unsigned char> decode(const Container &c) {
    return decode(c.begin(), c.end());
}

}