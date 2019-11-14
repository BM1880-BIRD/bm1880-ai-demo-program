#pragma once

#include <tuple>
#include <type_traits>
#include "mp/tuple.hpp"
#include "mp/boolean.hpp"

namespace pipeline {
    template <typename... ParamTypes>
    struct Interface {
        using input_type = std::tuple<ParamTypes...>;
    };

    namespace detail {
        template <typename Comp>
        struct ResolveInterface {
            using input_type = typename Comp::interface::input_type;
            using output_type = decltype(std::declval<Comp>().process(std::declval<input_type>()));
            template <typename Tuple>
            using sub_indices = typename mp::SubIndices<typename mp::tuple_decay<Tuple>::type, typename mp::tuple_decay<input_type>::type>::type;
        };

        template <typename Lhs, typename Rhs>
        struct ParameterCommon {
            static_assert(std::is_same<typename std::decay<Lhs>::type, typename std::decay<Rhs>::type>::value, "");
            using type = typename std::conditional<
                std::is_const<typename std::remove_reference<Lhs>::type>::value && std::is_const<typename std::remove_reference<Rhs>::type>::value,
                typename std::add_lvalue_reference<typename std::add_const<typename std::decay<Lhs>::type>::type>::type,
                typename std::conditional<
                    std::is_lvalue_reference<Lhs>::value && std::is_lvalue_reference<Rhs>::value,
                    typename std::add_lvalue_reference<typename std::decay<Lhs>::type>::type,
                    typename std::add_rvalue_reference<typename std::decay<Lhs>::type>::type
                >::type
            >::type;
        };

        template <typename Lhs, typename LhsIndices, typename Rhs, typename RhsIndices>
        struct ParameterSequenceCommon;

        template <typename Lhs, size_t... LhsIndices, typename Rhs, size_t... RhsIndices>
        struct ParameterSequenceCommon<Lhs, mp::Indices<LhsIndices...>, Rhs, mp::Indices<RhsIndices...>> {
            using type = std::tuple<typename ParameterCommon<
                typename std::tuple_element<LhsIndices, Lhs>::type,
                typename std::tuple_element<RhsIndices, Rhs>::type
            >::type...>;
        };

        template <typename Lhs, typename Rhs>
        struct InterfaceUnion {
        private:
            using lhs_decay = typename mp::tuple_decay<typename std::decay<Lhs>::type>::type;
            using rhs_decay = typename mp::tuple_decay<typename std::decay<Rhs>::type>::type;
            using lhs_only_index = typename mp::TupleDiffIndices<lhs_decay, rhs_decay>::type;
            using rhs_only_index = typename mp::TupleDiffIndices<rhs_decay, lhs_decay>::type;
            using lhs_common_index = typename mp::IndicesSubtract<typename mp::MakeIndices<0, std::tuple_size<Lhs>::value>::type, lhs_only_index>::type;
            using lhs_common_types = decltype(mp::tuple_subsequence(std::declval<lhs_decay>(), std::declval<lhs_common_index>()));
            using rhs_common_index = typename mp::SubIndices<rhs_decay, lhs_common_types>::type;

        public:
            using type = decltype(std::tuple_cat(
                mp::tuple_subsequence(std::declval<Lhs>(), std::declval<lhs_only_index>()),
                mp::tuple_subsequence(std::declval<Rhs>(), std::declval<rhs_only_index>()),
                std::declval<typename ParameterSequenceCommon<Lhs, lhs_common_index, Rhs, rhs_common_index>::type>()
            ));
        };
    }
}