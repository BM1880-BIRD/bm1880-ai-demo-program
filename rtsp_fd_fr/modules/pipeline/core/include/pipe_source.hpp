#pragma once

#include <tuple>
#include <memory>
#include "pipe_component.hpp"
#include "data_source.hpp"
#include "function_util.hpp"

namespace pipeline {
    template <typename SourceType>
    class Source;

    template <typename... Ts>
    class Source<std::tuple<Ts...>> : public Component<> {
    public:
        Source(std::shared_ptr<DataSource<Ts...>> src) : src_(std::move(src)) {}
        ~Source() { src_->close(); }

        std::tuple<Ts...> process(std::tuple<>) {
            auto value = src_->get();

            if (!value.has_value()) {
                throw std::runtime_error(std::string("pipeline::Source: failed to get data from ") + src_->source_name());
            }

            return helper(value, typename mp::MakeIndices<0, sizeof...(Ts)>::type());
        }

        void close() {
            src_->close();
        }

    private:
        template <size_t... Indices>
        std::tuple<Ts...> helper(mp::optional<Ts...> &data, mp::Indices<Indices...>) {
            return std::make_tuple(std::move(data.template value<Indices>())...);
        }

        std::shared_ptr<DataSource<Ts...>> src_;
    };

    template <typename S>
    class SourceWrap : public Source<typename S::output_type> {
    public:
        template <typename... Us>
        SourceWrap(Us &&...args) : Source<typename S::output_type>(std::make_shared<S>(std::forward<Us>(args)...)) {}
    };

    /**
     * Wrap a DataSource into a pipeline component.
     * pipeline::Source::process() invokes get function of the underlying DataSource and returns the value returned by get().
     * @return a shared pointer pointing to a pipeline::Source
     */
    template <typename S>
    std::shared_ptr<Source<typename S::output_type>> wrap_source(std::shared_ptr<S> src) {
        return std::make_shared<Source<typename S::output_type>>(std::move(src));
    }
}