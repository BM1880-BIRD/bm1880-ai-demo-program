#pragma once

#include <type_traits>

namespace mp {

    template <typename T>
    struct MemFuncInspec;

    template <typename Class, typename Ret, typename... Args>
    struct MemFuncInspec<Ret(Class::*)(Args...) const> {
        typedef std::tuple<Args...> argument_type;
        typedef Ret return_type;
    };

    template <typename Class, typename Ret, typename... Args>
    struct MemFuncInspec<Ret(Class::*)(Args...)> {
        typedef std::tuple<Args...> argument_type;
        typedef Ret return_type;
    };

    template <typename FuncType>
    using LambdaInspect = MemFuncInspec<decltype(&FuncType::operator())>;

}