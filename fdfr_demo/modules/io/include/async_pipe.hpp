#pragma once

#include <memory>
#include "data_sink.hpp"
#include "data_source.hpp"
#include "thread_runner.hpp"

template <typename T>
class AsyncPipe {
public:
    AsyncPipe(std::shared_ptr<DataSource<T>> &&src, std::shared_ptr<DataSink<T>> &&sink) : src_(std::move(src)), sink_(std::move(sink)) {
        pipe_thread_.start([this]() {
            T obj;
            if (false == src_->get(obj)) {
                pipe_thread_.stop();
            }
            fprintf(stderr, "piping one object\n");
            sink_->put(std::move(obj));
            fprintf(stderr, "piped one object\n");
        });
    }

    ~AsyncPipe() {
        pipe_thread_.stop();
    }

private:
    std::shared_ptr<DataSource<T>> src_;
    std::shared_ptr<DataSink<T>> sink_;
    ThreadRunner pipe_thread_;
};