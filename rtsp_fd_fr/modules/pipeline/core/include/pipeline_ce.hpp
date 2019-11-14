#pragma once

#include <tuple>
#include <type_traits>
#include "pipe_interface.hpp"
#include "pipe_sequence.hpp"
#include "mp/indices.hpp"

namespace pipeline {
    template <typename T>
    struct LackOfInput {
        using type = T;
        static_assert(std::is_same<T, std::tuple<>>::value, "input required by some components are not present");
    };
}