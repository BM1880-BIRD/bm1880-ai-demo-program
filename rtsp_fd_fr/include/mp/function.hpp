#pragma once

#include <type_traits>

namespace mp {
    /**
     * Inspect argument types and return type of a member function of any class.
     * typename MemFuncInspec<T>::argument_type is a tuple of argument types.
     * typename MemFuncInspec<T>::return_type is the return type of the function.
     */
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

    /**
     * Inspec argument types and return type of a lambda function, which is operator() of the type of the lambda.
     */
    template <typename FuncType>
    using LambdaInspect = MemFuncInspec<decltype(&FuncType::operator())>;
}