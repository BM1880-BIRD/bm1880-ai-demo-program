/**
 * This header defines the interface of pipeline::Sequence which concatenate components into a execution sequence.
 */
#pragma once

#include "pipe_interface.hpp"
#include "pipe_component.hpp"
#include "data_sink.hpp"
#include "mp/tuple.hpp"
#include "pipe_comp_builder.hpp"
#include "mp/filter.hpp"
#include "pipeline_ce.hpp"
#include "logging.hpp"

namespace pipeline {

    namespace detail {
        template <typename ParamTuple, typename Indices, typename ArgTuple, typename Offset>
        struct ForwardIndices;

        template <typename... ParamTypes, size_t... Indices, typename... ArgTypes>
        struct ForwardIndices<std::tuple<ParamTypes...>, mp::Indices<Indices...>, std::tuple<ArgTypes...>, std::integral_constant<size_t, sizeof...(ArgTypes)>> {
            using type = mp::Indices<>;
        };

        template <typename... ParamTypes, size_t... Indices, typename... ArgTypes, typename Offset>
        struct ForwardIndices<std::tuple<ParamTypes...>, mp::Indices<Indices...>, std::tuple<ArgTypes...>, Offset> {
        private:
            using query = mp::IndexOf<std::tuple<std::integral_constant<size_t, Indices>...>, Offset>;
            using param_type = typename std::tuple_element<query::value, std::tuple<ParamTypes..., int>>::type;
            static constexpr bool pick = !query::valid || !std::is_rvalue_reference<param_type>::value;
            using index = typename std::conditional<pick, mp::Indices<Offset::value>, mp::Indices<>>::type;
            using recurse = typename ForwardIndices<std::tuple<ParamTypes...>, mp::Indices<Indices...>, std::tuple<ArgTypes...>, std::integral_constant<size_t, Offset::value + 1>>::type;
        public:
            using type = decltype(std::declval<index>().concat(std::declval<recurse>()));
        };

        template <typename ParamTuple, typename Indices, typename ArgTuple, typename Offset>
        struct ReturnIndices;

        template <typename... ParamTypes, size_t... Indices, typename... ArgTypes>
        struct ReturnIndices<std::tuple<ParamTypes...>, mp::Indices<Indices...>, std::tuple<ArgTypes...>, std::integral_constant<size_t, sizeof...(ArgTypes)>> {
            using type = mp::Indices<>;
        };

        template <typename... ParamTypes, size_t... Indices, typename... ArgTypes, typename Offset>
        struct ReturnIndices<std::tuple<ParamTypes...>, mp::Indices<Indices...>, std::tuple<ArgTypes...>, Offset> {
        private:
            using query = mp::IndexOf<std::tuple<std::integral_constant<size_t, Indices>...>, Offset>;
            using param_type = typename std::tuple_element<query::value, std::tuple<ParamTypes..., int>>::type;
            static constexpr bool pick = (!query::valid || !std::is_rvalue_reference<param_type>::value) &&
                                        (std::is_rvalue_reference<typename std::tuple_element<Offset::value, std::tuple<ArgTypes...>>::type>::value);
            using index = typename std::conditional<pick, mp::Indices<Offset::value>, mp::Indices<>>::type;
            using recurse = typename ReturnIndices<std::tuple<ParamTypes...>, mp::Indices<Indices...>, std::tuple<ArgTypes...>, std::integral_constant<size_t, Offset::value + 1>>::type;
        public:
            using type = decltype(std::declval<index>().concat(std::declval<recurse>()));
        };

        template <typename... Comps>
        class SequenceImpl;

        template <typename Comp>
        class SequenceImpl<Comp> {
            template <typename Tuple>
            struct resolve {
                using decayT = typename std::decay<Tuple>::type;
                using input_type = typename ResolveInterface<Comp>::input_type;
                using output_type = typename ResolveInterface<Comp>::output_type;
                using arg_indices_t = typename mp::SubsetIndices<typename mp::tuple_decay<decayT>::type, typename mp::tuple_decay<input_type>::type>::type;
                using ret_indices_t = typename ReturnIndices<input_type, arg_indices_t, decayT, std::integral_constant<size_t, 0>>::type;
                using return_type = decltype(std::tuple_cat(mp::subtuple_construct(std::declval<typename mp::tuple_decay<decayT>::type>(), ret_indices_t()), std::declval<output_type>()));
            };

        public:
            using interface = typename Comp::interface;

            SequenceImpl(std::shared_ptr<Comp> comp) : comp_(std::move(comp)) {}

            template <typename Tuple>
            typename resolve<Tuple>::return_type process(Tuple &&tup) {
                auto result = comp_->process(mp::subtuple_convert<typename resolve<Tuple>::input_type>(tup, typename resolve<Tuple>::arg_indices_t()));

                return std::tuple_cat(mp::subtuple_construct(std::move(tup), typename resolve<Tuple>::ret_indices_t()), std::move(result));
            }

            void set_context(std::shared_ptr<Context> context) {
                comp_->set_context(context);
            }

            void close() {
                LOGD << "pipe comp: " << typeid(Comp).name() << " closing";
                comp_->close();
                LOGD << "pipe comp: " << typeid(Comp).name() << " closed";
            }

        private:
            std::shared_ptr<Comp> comp_;
        };

        template <typename Comp, typename... Rest>
        class SequenceImpl<Comp, Rest...> : private SequenceImpl<Rest...> {
            using parent_type = SequenceImpl<Rest...>;
            template <typename Tuple>
            struct resolve {
                using decayT = typename std::decay<Tuple>::type;
                using input_type = typename ResolveInterface<Comp>::input_type;
                using output_type = typename ResolveInterface<Comp>::output_type;
                using arg_indices_t = typename mp::SubsetIndices<typename mp::tuple_decay<decayT>::type, typename mp::tuple_decay<input_type>::type>::type;
                using forward_indices_t = typename ForwardIndices<input_type, arg_indices_t, decayT, std::integral_constant<size_t, 0>>::type;
                using forward_type = decltype(std::tuple_cat(mp::subtuple_forward(std::declval<decayT>(), forward_indices_t()), mp::tuple_move(std::declval<output_type>())));
            };

        public:
            typedef struct {
                using parent_input_type = typename parent_type::interface::input_type;
                using consuming_type = typename mp::tuple_filter<typename ResolveInterface<Comp>::input_type, std::is_rvalue_reference>::type;
                using borrowing_type = typename mp::tuple_filter<typename ResolveInterface<Comp>::input_type, std::is_lvalue_reference>::type;
                using producing_type = typename ResolveInterface<Comp>::output_type;
                using extra_input_type = decltype(mp::tuple_subsequence(
                    std::declval<parent_input_type>(),
                    typename mp::TupleDiffIndices<
                        typename mp::tuple_decay<parent_input_type>::type,
                        typename mp::tuple_decay<producing_type>::type
                    >::type()
                ));
                using input_type = decltype(std::tuple_cat(
                    std::declval<consuming_type>(),
                    std::declval<typename detail::InterfaceUnion<borrowing_type, extra_input_type>::type>()
                ));
            } interface;

            template <typename... Ts>
            SequenceImpl(std::shared_ptr<Comp> comp, Ts &&...args) : parent_type(std::forward<Ts>(args)...), comp_(std::move(comp)) {}

            template <typename U>
            decltype(std::declval<parent_type>().process(std::declval<typename resolve<U>::forward_type>()))
            process(U &&u) {
                auto result = comp_->process(mp::subtuple_convert<typename resolve<U>::input_type>(u, typename resolve<U>::arg_indices_t()));

                return parent_type::process(std::tuple_cat(mp::subtuple_forward(u, typename resolve<U>::forward_indices_t()), mp::tuple_move(result)));
            }

            void set_context(std::shared_ptr<Context> context) {
                comp_->set_context(context);
                parent_type::set_context(context);
            }

            void close() {
                LOGD << "pipe comp: " << typeid(Comp).name() << " closing";
                comp_->close();
                LOGD << "pipe comp: " << typeid(Comp).name() << " closed";
                parent_type::close();
            }

        private:
            std::shared_ptr<Comp> comp_;
        };
    }

    template <typename... Comps>
    using Sequence = detail::SequenceImpl<Comps...>;

    /**
     * Concatenate the sequence of input components into a pipeline::Sequence.
     * pipeline::Sequence::process calls the process functions of each components exactly once in the given order.
     * The input types of Sequence::process are the input types required by the components and not produced by their preceeding components.
     * The output types of Sequence::process are the output types of the components which are not consumed by their succeeding components.
     * @arg \c comps: one or more parameters are accepted. each parameter is a shared pointer pointing to a pipeline component.
     * @return a shared_ptr to a pipeline::Sequence
     */
    template <typename... Comps>
    std::shared_ptr<Sequence<typename detail::CompBuilder<Comps>::type...>> make_sequence(Comps &&... comps) {
        return std::make_shared<Sequence<typename detail::CompBuilder<Comps>::type...>>(detail::build_comp<Comps>(std::forward<Comps>(comps))...);
    }
}
