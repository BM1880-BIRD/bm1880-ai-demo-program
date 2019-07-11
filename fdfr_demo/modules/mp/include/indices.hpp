#pragma once

#include <tuple>
#include <type_traits>

namespace mp {
    template <typename From, typename Subtract>
    struct IndicesSubtract;

    template <size_t... Is>
    struct Indices {
        template <size_t... Js>
        Indices<Is..., Js...> concat(Indices<Js...>);

        using size = std::integral_constant<size_t, sizeof...(Is)>;
    };

    template <size_t... Is>
    struct IndicesSubtract<Indices<Is...>, Indices<>> {
        using type = Indices<Is...>;
    };

    template <size_t I, size_t... Is, size_t... Js>
    struct IndicesSubtract<Indices<I, Is...>, Indices<I, Js...>> {
        using type = typename IndicesSubtract<Indices<Is...>, Indices<Js...>>::type;
    };

    template <size_t I, size_t... Is, size_t J, size_t... Js>
    struct IndicesSubtract<Indices<I, Is...>, Indices<J, Js...>> {
        static_assert(I != J, "");
        using type = typename std::conditional<
            I < J,
            decltype(Indices<I>().concat(typename IndicesSubtract<Indices<Is...>, Indices<J, Js...>>::type())),
            typename IndicesSubtract<Indices<I, Is...>, Indices<Js...>>::type
        >::type;
    };

    template <size_t Begin, size_t End>
    struct MakeIndices {
        using type = decltype(std::declval<Indices<Begin>>().concat(std::declval<typename MakeIndices<Begin + 1, End>::type>()));
    };

    template <size_t End>
    struct MakeIndices<End, End> {
        using type = Indices<>;
    };

    template <typename Seq, typename T>
    struct IndexOf;

    template <typename T>
    struct IndexOf<std::tuple<>, T> {
        static constexpr bool valid = false;
        static constexpr bool unique = false;
        static constexpr size_t value = 0;
    };

    template <typename S, typename... Seq, typename T>
    struct IndexOf<std::tuple<S, Seq...>, T> {
        using recursion = IndexOf<std::tuple<Seq...>, T>;
        static constexpr bool valid = recursion::valid;
        static constexpr bool unique = recursion::unique;
        static constexpr size_t value = 1 + recursion::value;
    };

    template <typename... Seq, typename T>
    struct IndexOf<std::tuple<T, Seq...>, T> {
        static constexpr bool valid = true;
        static constexpr bool unique = !IndexOf<std::tuple<Seq...>, T>::valid;
        static constexpr size_t value = 0;
    };

    template <typename Super, typename Sub, size_t Offset>
    struct SubseqIndicesImpl;

    template <typename... Super, size_t Offset>
    struct SubseqIndicesImpl<std::tuple<Super...>, std::tuple<>, Offset> {
        static constexpr bool valid = true;
        static constexpr bool unique = true;
        using type = Indices<>;
    };

    template <typename S, typename... Sub, size_t Offset>
    struct SubseqIndicesImpl<std::tuple<>, std::tuple<S, Sub...>, Offset> {
        static constexpr bool valid = false;
        static constexpr bool unique = false;
        using type = Indices<>;
    };

    template <typename T, typename... Super, typename... Sub, size_t Offset>
    struct SubseqIndicesImpl<std::tuple<T, Super...>, std::tuple<T, Sub...>, Offset> {
        using recursion = SubseqIndicesImpl<std::tuple<Super...>, std::tuple<Sub...>, Offset + 1>;
        static constexpr bool valid = recursion::valid;
        static constexpr bool unique = !SubseqIndicesImpl<std::tuple<Super...>, std::tuple<T, Sub...>, Offset + 1>::valid && recursion::unique;
        using type = decltype(std::declval<Indices<Offset>>().concat(std::declval<typename recursion::type>()));
    };

    template <typename S, typename... Super, typename s, typename... Sub, size_t Offset>
    struct SubseqIndicesImpl<std::tuple<S, Super...>, std::tuple<s, Sub...>, Offset> {
        using recursion = SubseqIndicesImpl<std::tuple<Super...>, std::tuple<s, Sub...>, Offset + 1>;
        static constexpr bool valid = recursion::valid;
        static constexpr bool unique = recursion::unique;
        using type = typename recursion::type;
    };

    template <typename Super, typename Sub>
    using SubseqIndices = SubseqIndicesImpl<Super, Sub, 0>;

    template <typename Super, typename Sub>
    struct SubsetIndices;

    template <typename... Super>
    struct SubsetIndices<std::tuple<Super...>, std::tuple<>> {
        static constexpr bool valid = true;
        static constexpr bool unique = true;
        using type = Indices<>;
    };

    template <typename... Super, typename S, typename... Sub>
    struct SubsetIndices<std::tuple<Super...>, std::tuple<S, Sub...>> {
        using recursion = SubsetIndices<std::tuple<Super...>, std::tuple<Sub...>>;
        using query = IndexOf<std::tuple<Super...>, S>;
        static constexpr bool valid = query::valid && recursion::valid && !IndexOf<std::tuple<Sub...>, S>::valid;
        static constexpr bool unique = query::unique && recursion::unique;
        using type = decltype(std::declval<Indices<query::value>>().concat(std::declval<typename recursion::type>()));
    };

    template <typename Super, typename Sub>
    struct SubIndices {
    private:
        using subset_indices = SubsetIndices<Super, Sub>;
        using subseq_indices = SubseqIndices<Super, Sub>;
    public:
        using type = typename std::conditional<
            subset_indices::valid && subset_indices::unique,
            typename subset_indices::type,
            typename std::conditional<
                subseq_indices::valid && subseq_indices::unique,
                typename subseq_indices::type,
                void
            >::type
        >::type;
    };
}
