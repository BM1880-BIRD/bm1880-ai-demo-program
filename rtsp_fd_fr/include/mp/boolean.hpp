#pragma once

#include <type_traits>

namespace mp {
    /// Helper class computing "boolean and" in compile-time
    template <typename... Booleans>
    struct And;

    template <>
    struct And<> : public std::true_type {};

    template <typename... Booleans>
    struct And<std::true_type, Booleans...> : public And<Booleans...> {};

    template <typename... Booleans>
    struct And<std::false_type, Booleans...> : public std::false_type {};
}