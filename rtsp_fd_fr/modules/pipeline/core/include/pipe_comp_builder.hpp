#pragma once

#include <memory>
#include <type_traits>

namespace pipeline {
    namespace detail {
        template <typename T, typename U>
        class CompBuilderImpl;

        template <typename T>
        class CompBuilderImpl<std::shared_ptr<T>, void> {
        public:
            typedef T type;
            static std::shared_ptr<T> build(std::shared_ptr<T> t) {
                return t;
            }
        };

        template <typename T>
        using CompBuilder = CompBuilderImpl<typename std::decay<T>::type, void>;

        template <typename T>
        std::shared_ptr<typename CompBuilder<T>::type> build_comp(T &&t) {
            return CompBuilder<T>::build(std::forward<T>(t));
        }
    }
}