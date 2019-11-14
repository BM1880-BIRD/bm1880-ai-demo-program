#pragma once

#include <tuple>
#include <type_traits>

template <typename Ret, typename Func, typename Tuple>
class Invoke {
};

template <typename Ret, typename Func>
class Invoke<Ret, Func, std::tuple<>> {
public:
    template <typename... Unpacked>
    inline static Ret invoke(Func &&f, Unpacked &&...unpacked, std::tuple<> &&t) {
        return f(std::forward<Unpacked>(unpacked)...);
    }
};

template <typename Ret, typename Func, typename T, typename... Args>
class Invoke<Ret, Func, std::tuple<T, Args...>> {
public:
    template <typename... Unpacked>
    inline static Ret invoke(Func &&f, Unpacked &&...unpacked, std::tuple<T, Args...> &&t) {
        return Invoke<Ret, Func, std::tuple<Args...>>::template invoke<Unpacked..., T>(std::forward<Func>(f),
                                                                           std::forward<Unpacked>(unpacked)...,
                                                                           std::move(std::get<0>(t)),
                                                                           std::move(reinterpret_cast<std::tuple<Args...> &>(t)));
    }
};