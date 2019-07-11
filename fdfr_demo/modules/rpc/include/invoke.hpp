#pragma once

template <typename Ret, typename... ArgTypes>
class DecodeInvoke;

template <typename Ret>
class DecodeInvoke<Ret> {
public:
    template <typename FuncType, typename Iterator, typename... Args>
    static Ret invoke(FuncType &&f, Iterator, Iterator, Args &&...args) {
        return f(std::forward<Args>(args)...);
    }
};

template <typename Ret, typename ArgType, typename... ArgTypes>
class DecodeInvoke<Ret, ArgType, ArgTypes...> {
public:
    template <typename FuncType, typename Iterator, typename... Args>
    static Ret invoke(FuncType &&f, Iterator begin, Iterator end, Args &&...args) {
        typename std::decay<ArgType>::type t;
        begin = decode<decltype(t)>(begin, end, t);
        return DecodeInvoke<Ret, ArgTypes...>::invoke(f, begin, end, std::forward<Args>(args)..., std::move(t));
    }
};

template <typename Iterable, typename Ret, typename T, typename U, typename... ArgTypes>
Ret decode_invoke(Ret (T::*f)(ArgTypes...), U &obj, Iterable &args) {
    return DecodeInvoke<Ret, ArgTypes...>::invoke(std::mem_fn(f), args.begin(), args.end(), obj);
}