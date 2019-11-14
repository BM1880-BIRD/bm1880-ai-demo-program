#pragma once

#include <memory>
#include <tuple>
#include <functional>
#include "pipe_component.hpp"
#include "function_util.hpp"
#include "pipe_comp_builder.hpp"
#include "mp/function.hpp"

namespace pipeline {
    namespace detail {
        template <typename FuncType, typename InType, typename OutType>
        class Lambda;

        template <typename FuncType, typename... InTypes, typename... OutTypes>
        class Lambda<FuncType, std::tuple<InTypes...>, std::tuple<OutTypes...>> : public Component<InTypes...> {
        public:
            Lambda(FuncType &&func) : func_(std::move(func)) {}

            std::tuple<OutTypes...> process(std::tuple<InTypes...> args) {
                return Invoke<std::tuple<OutTypes...>, FuncType &, std::tuple<InTypes...>>::invoke(func_, std::move(args));
            }

        private:
            FuncType func_;
        };

        template <typename FuncType, typename... InTypes, typename OutType>
        class Lambda<FuncType, std::tuple<InTypes...>, OutType> : public Component<InTypes...> {
        public:
            Lambda(FuncType &&func) : func_(std::move(func)) {}

            std::tuple<OutType> process(std::tuple<InTypes...> args) {
                return helper(args, typename mp::MakeIndices<0, sizeof...(InTypes)>::type());
            }

        private:
            template <size_t... Indices>
            inline std::tuple<OutType> helper(std::tuple<InTypes...> &args, mp::Indices<Indices...>) {
                return std::make_tuple(func_(std::forward<typename std::tuple_element<Indices, std::tuple<InTypes...>>::type>(std::get<Indices>(args))...));
            }

            FuncType func_;
        };

        template <typename FuncType, typename... InTypes>
        class Lambda<FuncType, std::tuple<InTypes...>, void> : public Component<InTypes...> {
        public:
            Lambda(FuncType &&func) : func_(std::move(func)) {}

            std::tuple<> process(std::tuple<InTypes...> args) {
                helper(args, typename mp::MakeIndices<0, sizeof...(InTypes)>::type());
                return {};
            }

        private:
            template <size_t... Indices>
            inline void helper(std::tuple<InTypes...> &args, mp::Indices<Indices...>) {
                func_(std::forward<typename std::tuple_element<Indices, std::tuple<InTypes...>>::type>(std::get<Indices>(args))...);
            }

            FuncType func_;
        };
    }

    /**
     * Wrap a function object (an object for which the function call operator is defined) into a pipeline component
     * pipeline::detail::Lambda::process() invokes the given function object and returns its result.
     * @arg \c func: a function object
     * @return a shared pointer to an instance of pipeline::detail::Lambda
     */
    template <typename FuncType>
    std::shared_ptr<detail::Lambda<FuncType, typename mp::LambdaInspect<FuncType>::argument_type, typename mp::LambdaInspect<FuncType>::return_type>> from_lambda(FuncType &&func) {
        return std::make_shared<detail::Lambda<FuncType, typename mp::LambdaInspect<FuncType>::argument_type, typename mp::LambdaInspect<FuncType>::return_type>>(std::move(func));
    }

    namespace detail {
        /**
         * Partial specialization for CompBuilderImpl that calls from_lambda to wrap given function object.
         * When type T is not a function object, the third parameter of std::conditional cannot be resolved hence this specialization is ignored.
         */
        template <typename T>
        class CompBuilderImpl<T, typename std::conditional<true, void, decltype(&T::operator())>::type> {
        public:
            typedef typename decltype(from_lambda(std::declval<T>()))::element_type type;
            static std::shared_ptr<type> build(T &&f) {
                return from_lambda(std::forward<T>(f));
            }
        };
    }
}