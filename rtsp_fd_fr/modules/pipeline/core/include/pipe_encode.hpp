#pragma once

#include <memory>
#include <tuple>
#include <vector>
#include "memory/bytes.hpp"
#include "memory/encoding.hpp"
#include "pipe_component.hpp"

namespace pipeline {

    template <typename... Ts>
    class Encode : public Component<Ts &&...> {
    public:
        std::tuple<std::list<memory::Bytes>> process(std::tuple<Ts &&...> args) {
            auto list = memory::Encoding<std::tuple<Ts...>>::encode(std::move(args));
            std::list<memory::Bytes> data;
            for (auto &e : list) {
                data.emplace_back(std::move(e));
            }
            return std::make_tuple(std::move(data));
        }
    };

    template <typename... Ts>
    class Decode : public Component<std::list<memory::Bytes> &&> {
    public:
        std::tuple<Ts...> process(std::tuple<std::list<memory::Bytes>&&> args) {
            std::tuple<Ts...> tuple;
            memory::Encoding<std::tuple<Ts...>>::decode(std::get<0>(args), tuple);
            return tuple;
        }
    };

    template <typename... Ts>
    class PartialDecode : public Component<std::list<memory::Bytes> &> {
    public:
        std::tuple<Ts...> process(std::tuple<std::list<memory::Bytes>&> args) {
            std::tuple<Ts...> tuple;
            memory::Encoding<std::tuple<Ts...>>::decode(std::get<0>(args), tuple);
            return tuple;
        }
    };
}