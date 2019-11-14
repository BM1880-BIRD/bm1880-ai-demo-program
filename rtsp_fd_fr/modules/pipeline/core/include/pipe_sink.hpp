#pragma once

#include <tuple>
#include <memory>
#include "pipe_component.hpp"
#include "data_sink.hpp"
#include "function_util.hpp"

namespace pipeline {
    template <typename T>
    class Sink;

    template <typename... Ts>
    class Sink<std::tuple<Ts...>> : public Component<Ts &&...> {
    public:
        Sink(std::shared_ptr<DataSink<Ts...>> sink) : sink_(std::move(sink)) {}
        ~Sink() { sink_->close(); }

        std::tuple<> process(std::tuple<Ts &&...> data) {
            using indices_t = typename mp::MakeIndices<0, sizeof...(Ts)>::type;
            if (!helper(data, indices_t())) {
                throw std::runtime_error(std::string("pipeline::Sink: failed to put data into [") + sink_->sink_name() + "]");
            }
            return {};
        }

        void close() {
            sink_->close();
        }

    private:
        template <size_t... Indices>
        bool helper(std::tuple<Ts &&...> &data, mp::Indices<Indices...>) {
            return sink_->put(std::move(std::get<Indices>(data))...);
        }

        std::shared_ptr<DataSink<Ts...>> sink_;
    };

    template <typename S>
    class SinkWrap : public Sink<typename S::input_type> {
    public:
        template <typename... Us>
        SinkWrap(Us &&...args) : Sink<typename S::input_type>(std::make_shared<S>(std::forward<Us>(args)...)) {}
    };

    /**
     * Wrap a DataSink into a pipeline component.
     * pipeline::Sink::process() consumes data of input types of the underlying DataSink, and invokes its put function, putting data into the sink.
     * @return a shared pointer pointing to a pipeline::Sink
     */
    template <typename S>
    std::shared_ptr<Sink<typename S::input_type>> wrap_sink(std::shared_ptr<S> sink) {
        return std::make_shared<Sink<typename S::input_type>>(std::move(sink));
    }
}
