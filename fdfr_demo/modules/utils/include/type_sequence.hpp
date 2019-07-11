#pragma once

#include <tuple>
#include <type_traits>

template <typename L, typename R>
struct IsSubseqOf;

template <>
struct IsSubseqOf<std::tuple<>, std::tuple<>> {
    static constexpr bool value = true;
};

template <typename L, typename... Ls>
struct IsSubseqOf<std::tuple<L, Ls...>, std::tuple<>> {
    static constexpr bool value = false;
};

template <typename... Ls, typename R, typename... Rs>
struct IsSubseqOf<std::tuple<Ls...>, std::tuple<R, Rs...>> {
    static constexpr bool value = IsSubseqOf<std::tuple<Ls...>, std::tuple<Rs...>>::value;
};

template <typename T, typename... Ls, typename... Rs>
struct IsSubseqOf<std::tuple<T, Ls...>, std::tuple<T, Rs...>> {
    static constexpr bool value = IsSubseqOf<std::tuple<Ls...>, std::tuple<Rs...>>::value;
};

template <typename... Seqs>
struct LargestSequence;

template <typename T>
struct LargestSequence<T> {
    typedef T type;
};

template <typename... First, typename... Second, typename... Rest>
struct LargestSequence<std::tuple<First...>, std::tuple<Second...>, Rest...> {
    typedef typename std::conditional<
        IsSubseqOf<std::tuple<First...>, std::tuple<Second...>>::value,
        typename LargestSequence<std::tuple<Second...>, Rest...>::type,
        typename std::conditional<
            IsSubseqOf<std::tuple<Second...>, typename LargestSequence<std::tuple<First...>, Rest...>::type>::value,
            typename LargestSequence<std::tuple<First...>, Rest...>::type,
            void
        >::type
    >::type type;
    static constexpr bool valid = !std::is_same<type, void>::value;
};

template <typename... Seqs>
struct SmallestSequence;

template <typename T>
struct SmallestSequence<T> {
    typedef T type;
};

template <typename... First, typename... Second, typename... Rest>
struct SmallestSequence<std::tuple<First...>, std::tuple<Second...>, Rest...> {
    typedef typename std::conditional<
        IsSubseqOf<std::tuple<First...>, std::tuple<Second...>>::value,
        typename SmallestSequence<std::tuple<First...>, Rest...>::type,
        typename std::conditional<
            IsSubseqOf<std::tuple<Second...>, std::tuple<First...>>::value,
            typename SmallestSequence<std::tuple<Second...>, Rest...>::type,
            void
        >::type
    >::type type;
    static constexpr bool valid = !std::is_same<type, void>::value;
};