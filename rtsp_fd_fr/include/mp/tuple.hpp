#pragma once

#include <tuple>
#include "indices.hpp"

namespace mp {

    template <typename Tuple>
    struct tuple_decay_impl;

    template <typename... Types>
    struct tuple_decay_impl<std::tuple<Types...>> {
        using type = std::tuple<typename std::decay<Types>::type...>;
    };

    template <typename Tuple>
    struct tuple_element_unique;

    template <>
    struct tuple_element_unique<std::tuple<>> {
        static constexpr bool value = true;
    };

    template <typename T, typename... Ts>
    struct tuple_element_unique<std::tuple<T, Ts...>> {
        static constexpr bool value = !IndexOf<std::tuple<Ts...>, T>::valid && tuple_element_unique<std::tuple<Ts...>>::value;
    };

    template <typename T>
    using tuple_decay = tuple_decay_impl<typename std::decay<T>::type>;

    template <typename T, typename... Types>
    T &tuple_get(std::tuple<Types...> &tuple) {
        return std::get<IndexOf<std::tuple<Types...>, T>::value>(tuple);
    }

    template <typename T, typename... Types>
    const T &tuple_get(const std::tuple<Types...> &tuple) {
        return std::get<IndexOf<std::tuple<Types...>, T>::value>(tuple);
    }

    template <typename T, size_t... Idx>
    inline std::tuple<typename std::tuple_element<Idx, typename std::decay<T>::type>::type...> tuple_subsequence(T &&tuple, Indices<Idx...>) {
        return std::make_tuple(std::move(std::get<Idx>(tuple))...);
    }

    template <typename T, size_t... Idx>
    inline std::tuple<typename std::decay<typename std::tuple_element<Idx, T>::type>::type...> subtuple_construct(T &&tuple, Indices<Idx...>) {
        return std::make_tuple<typename std::decay<typename std::tuple_element<Idx, T>::type>::type...>(std::move(std::get<Idx>(tuple))...);
    }

    template <typename Tup, size_t... Idx>
    inline std::tuple<typename std::remove_reference<typename std::tuple_element<Idx, typename std::decay<Tup>::type>::type>::type &&...>
    subtuple_move(Tup &&tup, Indices<Idx...>) {
        return std::forward_as_tuple(std::move(std::get<Idx>(tup))...);
    }

    template <typename... Types>
    inline std::tuple<typename std::remove_reference<Types>::type &&...> tuple_move(std::tuple<Types...> &&t) {
        return subtuple_move(std::move(t), typename MakeIndices<0, sizeof...(Types)>::type());
    }

    template <typename... Types>
    inline std::tuple<typename std::remove_reference<Types>::type &&...> tuple_move(std::tuple<Types...> &t) {
        return subtuple_move(std::move(t), typename MakeIndices<0, sizeof...(Types)>::type());
    }

    template <typename Tup, size_t... Idx>
    inline std::tuple<typename std::tuple_element<Idx, typename std::decay<Tup>::type>::type...>
    subtuple_forward(Tup &&tup, Indices<Idx...>) {
        return std::forward_as_tuple(((typename std::tuple_element<Idx, typename std::decay<Tup>::type>::type)std::get<Idx>(tup))...);
    }

    template <typename... Types>
    inline std::tuple<Types &&...> tuple_forward(std::tuple<Types...> &&t) {
        return subtuple_forward(std::move(t), typename MakeIndices<0, sizeof...(Types)>::type());
    }

    template <typename RetType, typename InType, size_t... Idx>
    inline RetType tuple_convert(InType &&tup, Indices<Idx...>) {
        return std::forward_as_tuple(static_cast<typename std::tuple_element<Idx, typename std::decay<RetType>::type>::type>(std::get<Idx>(tup))...);
    }

    template <typename RetType, typename InType, size_t... Idx>
    inline RetType subtuple_convert(InType &&tup, Indices<Idx...>) {
        return tuple_convert<RetType>(
            subtuple_forward(std::forward<InType>(tup), Indices<Idx...>()),
            typename MakeIndices<0, std::tuple_size<RetType>::value>::type()
        );
    }

    template <typename RetType, typename InType>
    inline RetType subtuple_convert(InType &&tup) {
        using indices_t = typename mp::SubIndices<typename mp::tuple_decay<InType>::type, typename mp::tuple_decay<RetType>::type>::type;
        return subtuple_convert<RetType>(std::forward<InType>(tup), indices_t());
    }

    template <typename ToType, typename FromType>
    inline ToType tuple_cast(FromType &&tuple) {
        using indices_t = typename SubIndices<FromType, ToType>::type;
        static_assert(!std::is_same<indices_t, void>::value, "tuple_cast: invalid conversion");
        return tuple_subsequence(std::move(tuple), indices_t());
    }

    template <typename T, typename... Ts>
    inline std::tuple<Ts...> tuple_pop_first(std::tuple<T, Ts...> &&tuple) {
        typedef typename MakeIndices<1, sizeof...(Ts) + 1>::type indices_t;
        return tuple_subsequence(std::move(tuple), indices_t());
    }

    template <typename ToType, typename FromType>
    struct TupleCastValid {
        using indices_t = typename SubIndices<FromType, ToType>::type;
        static constexpr bool value = !std::is_same<indices_t, void>::value;
    };

    template <typename Tuple, template<typename> class Transform>
    struct tuple_map;

    template <typename... Types, template<typename> class Transform>
    struct tuple_map<std::tuple<Types...>, Transform> {
        using type = std::tuple<Transform<Types>...>;
    };

    template <typename Tuple, template<typename> class Predicate>
    struct tuple_filter;

    template <template<typename> class Predicate>
    struct tuple_filter<std::tuple<>, Predicate> {
        using type = std::tuple<>;
    };

    template <typename T, typename... Types, template<typename> class Predicate>
    struct tuple_filter<std::tuple<T, Types...>, Predicate> {
        using type = decltype(std::tuple_cat(
            std::declval<typename std::conditional<Predicate<T>::value, std::tuple<T>, std::tuple<>>::type>(),
            std::declval<typename tuple_filter<std::tuple<Types...>, Predicate>::type>()
        ));
    };

    template <typename Super, typename Sub, typename Index>
    struct TupleDiffIndicesImpl {
        using sub_index = IndexOf<Sub, typename std::tuple_element<Index::value, Super>::type>;
        using recurse = TupleDiffIndicesImpl<
            Super,
            decltype(tuple_subsequence(
                std::declval<Sub>(),
                typename IndicesSubtract<
                    typename MakeIndices<0, std::tuple_size<Sub>::value>::type,
                    typename std::conditional<sub_index::valid, Indices<sub_index::value>, Indices<>>::type
                >::type()
            )),
            std::integral_constant<size_t, Index::value + 1>
        >;
        using type = decltype(std::declval<typename std::conditional<sub_index::valid, Indices<>, Indices<Index::value>>::type>().concat(typename recurse::type()));
    };

    template <typename... Super, typename Sub>
    struct TupleDiffIndicesImpl<std::tuple<Super...>, Sub, std::integral_constant<size_t, sizeof...(Super)>> {
        using type = Indices<>;
    };

    template <typename Super, typename Sub>
    using TupleDiffIndices = TupleDiffIndicesImpl<Super, Sub, std::integral_constant<size_t, 0>>;
}