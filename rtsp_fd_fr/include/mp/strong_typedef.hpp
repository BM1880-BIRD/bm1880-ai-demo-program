#pragma once

#include <utility>

/**
 * create a distinct type from a given type which can be converted from/to the original type.
 * Can be used to create a new type for passing data in the pipeline since pipeline doesn't allow duplicating types.
 */
#define MP_STRONG_TYPEDEF(T, D)                                         \
    struct D {                                                          \
        T t;                                                            \
        D() {}                                                          \
        D(const T &in) : t(in) {}                              \
        D(T &&in) : t(std::move(in)) {}                        \
        D(const D &in) : t(in.t) {}                                     \
        D(D &&in) : t(std::move(in.t)) {}                               \
        D &operator=(const T &in) {t = t; return *this;}             \
        D &operator=(T &&in) {t = std::move(t); return *this;}       \
        D &operator=(const D &in) {t = in.t; return *this;}             \
        D &operator=(D &&in) {t = std::move(in.t); return *this;}       \
        operator const T &() const {return t;}                          \
        operator T &() {return t;}                                      \
        bool operator==(const D &rhs) const { return t == rhs.t; }      \
        bool operator<(const D &rhs) const { return t < rhs.t; }        \
    };
