#pragma once

#include <tuple>
#include <memory>
#include "data_sink.hpp"
#include "pipe_context.hpp"
#include "pipe_interface.hpp"
#include "mp/boolean.hpp"

namespace pipeline {

    template <typename... InTypes>
    class Component {
        template <typename T>
        using is_const_reference = std::is_const<typename std::remove_reference<T>::type>;
    public:
        static_assert(
            mp::And<std::integral_constant<bool,
                ((std::is_lvalue_reference<InTypes>::value) ||
                (!is_const_reference<InTypes>::value && std::is_rvalue_reference<InTypes>::value))>...
            >::value,
            "Input type of Component must be either lvalue reference or non-const rvalue reference"
        );
        using interface = Interface<InTypes...>;

        void set_context(std::shared_ptr<Context> context) {
            context_ = context;
        }

        void close() {}

    protected:
        std::shared_ptr<Context> context_;
    };

}
