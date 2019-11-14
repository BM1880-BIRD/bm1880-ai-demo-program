#pragma once

#include <cstdio>
#include <memory>
#include <type_traits>
#include "pipe_sequence.hpp"
#include "pipe_context.hpp"
#include "pipeline_ce.hpp"

namespace pipeline {

    class Pipeline {
    public:
        virtual ~Pipeline() {}

        /**
         * repeatedly calls execute_once() until it returns false
         */
        void execute() {
            while (execute_once());
        }

        virtual bool execute_once() = 0;
        virtual void set_context(std::shared_ptr<Context> context) = 0;
        virtual void close() = 0;
    };

    namespace detail {
        template <typename Comp>
        using void_executable = std::integral_constant<bool, std::is_same<std::tuple<>, typename detail::ResolveInterface<Comp>::input_type>::value>;

        template <typename Comp, typename V>
        class PipelineImpl {
            static typename LackOfInput<typename detail::ResolveInterface<Comp>::input_type>::type ce_msg;

        public:
            template <typename... Us>
            PipelineImpl(Us &&...us) {}

            void execute() {
            }

            bool execute_once() {
                return false;
            }

            void set_context(std::shared_ptr<Context> context) {
            }

            void close() {
            }
        };

        template <typename Comp>
        class PipelineImpl<Comp, typename std::enable_if<void_executable<Comp>::value>::type> : public Pipeline {
        public:
            PipelineImpl(std::shared_ptr<Comp> comp) : comp_(comp) {}

            /**
             * calls process of the underlying component, return false if exception thrown during the process.
             */
            bool execute_once() override {
                try {
                    comp_->process(std::tuple<>());
                    return true;
                } catch (std::runtime_error &ex) {
                    fprintf(stderr, "Exception in executing pipeline:\n%s\n", ex.what());
                    return false;
                }
            }

            void set_context(std::shared_ptr<Context> context) override {
                comp_->set_context(context);
            }

            void close() override {
                comp_->close();
            }

        private:
            std::shared_ptr<Comp> comp_;
        };
    }

    /**
     * @brief Creates a pipeline::Pipeline contains given components
     *
     * Forwards the input components to pipeline::make_sequence and creates a pipeline::Pipeline holding the pipeline::Sequence returned by make_sequence
     */
    template <typename... Ts>
    std::shared_ptr<Pipeline> make(Ts &&... values) {
        typedef typename decltype(make_sequence(std::declval<Ts>()...))::element_type sequence_type;
        return std::make_shared<detail::PipelineImpl<sequence_type, void>>(make_sequence(std::forward<Ts>(values)...));
    }

}