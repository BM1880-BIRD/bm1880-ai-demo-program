#pragma once

#include <tuple>

namespace mp {
    template <template <typename T> class Pred, typename T>
    struct Filter;

    template <template <typename T> class Pred>
    struct Filter<Pred, std::tuple<>> {
        using type = std::tuple<>;
    };

    template <template <typename T> class Pred, typename First, typename... Rest>
    struct Filter<Pred, std::tuple<First, Rest...>> {
        using rest_filtered = typename Filter<Pred, std::tuple<Rest...>>::type;
        using type = typename std::conditional<Pred<First>::value, decltype(std::tuple_cat(std::declval<std::tuple<First>>(), std::declval<rest_filtered>())), rest_filtered>::type;
    };
}