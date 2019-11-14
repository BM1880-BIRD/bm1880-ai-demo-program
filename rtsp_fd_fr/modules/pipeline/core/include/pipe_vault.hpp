#pragma once

#include "pipe_component.hpp"

namespace pipeline {

template <typename T>
struct vault_t {
    T value;
};

template <typename T>
class VaultPush : public Component<const T &> {
public:
    std::tuple<vault_t<T>> process(std::tuple<const T &> tup) {
        return std::make_tuple<vault_t<T>>({std::get<0>(tup)});
    }
};

template <typename T>
class VaultPop : public Component<vault_t<T> &&> {
public:
    std::tuple<T> process(std::tuple<vault_t<T> &&> tup) {
        return std::make_tuple<T>(std::move(std::get<0>(tup).value));
    }
};

}