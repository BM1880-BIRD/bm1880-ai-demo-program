#pragma once

#include <tuple>
#include <memory>
#include "mp/boolean.hpp"
#include "data_sink.hpp"
#include "pipe_component.hpp"
#include "pipe_context.hpp"
#include "mp/type_sequence.hpp"
#include "mp/tuple.hpp"
#include "pipe_comp_builder.hpp"

namespace pipeline {
    struct switch_default {};

    namespace detail {
        template <typename TagType, typename... Ts>
        class SwitchImpl;

        template <typename TagType, typename Comp>
        class SwitchImpl<TagType, Comp> {
        public:
            using interface = typename Comp::interface;

            template <typename T, typename std::enable_if<std::is_same<typename std::decay<T>::type::first_type, TagType>::value, int>::type = 0>
            SwitchImpl(T &&comp) :
            wildcard_(false),
            tag_(std::move(comp.first)),
            comp_(CompBuilder<typename std::decay<T>::type::second_type>::build(std::get<1>(std::forward<T>(comp)))) {}

            template <typename T, typename std::enable_if<std::is_same<typename std::decay<T>::type::first_type, switch_default>::value, int>::type = 0>
            SwitchImpl(T &&comp) :
            wildcard_(true),
            comp_(CompBuilder<typename std::decay<T>::type::second_type>::build(std::get<1>(std::forward<T>(comp)))) {}

            template <typename ReturnType, typename Tuple>
            ReturnType process(const TagType &tag, Tuple &&tuple) {
                if (tag != tag_ && !wildcard_) {
                    throw std::runtime_error("pipeline::detail::Switch: tag is invalid.");
                }
                using input_type = typename ResolveInterface<Comp>::input_type;

                return mp::tuple_cast<ReturnType>(comp_->process(mp::subtuple_convert<input_type>(tuple)));
            }

            void set_context(std::shared_ptr<Context> context) {
                comp_->set_context(context);
            }

            void close() {
                comp_->close();
            }

        private:
            TagType tag_;
            bool wildcard_;
            std::shared_ptr<Comp> comp_;
        };

        template <typename TagType, typename Comp, typename... Rest>
        class SwitchImpl<TagType, Comp, Rest...> : private SwitchImpl<TagType, Rest...> {
        public:
            typedef struct {
                using comp_input_type = typename Comp::interface::input_type;
                using parent_input_type = typename SwitchImpl<TagType, Rest...>::interface::input_type;
                using input_type = typename InterfaceUnion<comp_input_type, parent_input_type>::type;
            } interface;

            template <typename T, typename... Us>
            SwitchImpl(T &&comp, Us &&...args) :
            SwitchImpl<TagType, Rest...>(std::forward<Us>(args)...),
            tag_(std::move(comp.first)),
            comp_(CompBuilder<typename std::decay<T>::type::second_type>::build(std::get<1>(std::forward<T>(comp)))) {}

            template <typename ReturnType, typename Tuple>
            ReturnType process(const TagType &tag, Tuple &&tuple) {
                if (tag == tag_) {
                    using input_type = typename ResolveInterface<Comp>::input_type;

                    return mp::tuple_cast<ReturnType>(comp_->process(mp::subtuple_convert<input_type>(tuple)));
                } else {
                    return SwitchImpl<TagType, Rest...>::template process<ReturnType>(tag, std::move(tuple));
                }
            }

            void set_context(std::shared_ptr<Context> context) {
                comp_->set_context(context);
                SwitchImpl<TagType, Rest...>::set_context(context);
            }

            void close() {
                comp_->close();
                SwitchImpl<TagType, Rest...>::close();
            }

        private:
            TagType tag_;
            std::shared_ptr<Comp> comp_;
        };

        template <typename TagType, typename... Ts>
        class Switch : private SwitchImpl<TagType, Ts...> {
        public:
            typedef struct {
                using input_type = decltype(std::tuple_cat(
                    std::declval<std::tuple<TagType &&>>(),
                    std::declval<typename SwitchImpl<TagType, Ts...>::interface::input_type>())
                );
            } interface;

            template <typename... Us>
            Switch(Us &&...us) : SwitchImpl<TagType, Ts...>(std::forward<Us>(us)...) {}

            template <typename Tuple>
            using RemoveTagType = decltype(mp::tuple_pop_first(std::declval<Tuple>()));
            template <typename Tuple>
            using ReturnType = typename mp::SmallestSequence<typename ResolveInterface<Ts>::output_type...>::type;

            template <typename Tuple>
            ReturnType<Tuple>
            process(Tuple &&tuple) {
                auto &tag = std::get<0>(tuple);
                return SwitchImpl<TagType, Ts...>::template process<ReturnType<Tuple>>(
                    tag,
                    mp::subtuple_convert<typename SwitchImpl<TagType, Ts...>::interface::input_type>(tuple)
                );
            }

            void set_context(std::shared_ptr<Context> context) {
                SwitchImpl<TagType, Ts...>::set_context(context);
            }

            void close() {
                SwitchImpl<TagType, Ts...>::close();
            }
        };
    }

    /**
     * @brief Creates a pipeline::detail::Switch which mimics the switch-case statement.
     *
     * A pipeline::detail::Switch is constructed with one or more sub-components, each sub-component is associated with a value of TagType.
     * pipeline::detail::Switch::process() consumes a value of TagType and use the value to determine which sub-component will be executed in the process.
     * @arg \c s: a sequence of pairs, the first value of each pair is a tag value, the second value of each pair is a component associated with that value.
     * @arg \c tag: pipeline::switch_default can also be used as a tag value, and matches with any value.
     * @return a shared pointer pointing to an instance of pipeline::detail::Switch
     */
    template <typename TagType, typename... Ts>
    std::shared_ptr<detail::Switch<TagType, typename detail::CompBuilder<typename std::decay<Ts>::type::second_type>::type...>> make_switch(Ts &&... s) {
        return std::make_shared<detail::Switch<TagType, typename detail::CompBuilder<typename std::decay<Ts>::type::second_type>::type...>>(std::move(s)...);
    }
}
