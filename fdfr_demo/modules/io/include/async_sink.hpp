#pragma once

#include <memory>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <type_traits>
#include <atomic>
#include <tuple>
#include "data_sink.hpp"
#include "thread_runner.hpp"
#include "function_util.hpp"

template <typename SinkType>
class AsyncSink;

template <typename... Ts>
class AsyncSink<DataSink<Ts...>> : public DataSink<Ts...> {
public:
    AsyncSink(std::shared_ptr<DataSink<Ts...>> &&sink) : sink_(std::move(sink)) {
        writer_thread_.start([this]() {
            if (!write()) {
                writer_thread_.stop();
            }
        });
    }
    ~AsyncSink() {
        writer_thread_.stop();
        available_.notify_all();
    }

    bool put(Ts&&...) override;
    bool is_opened() override {
        return sink_->is_opened();
    }
    void close() override;

private:
    bool write();

    std::shared_ptr<DataSink<Ts...>> sink_;
    size_t capacity_;
    std::mutex lock_;
    std::condition_variable available_;
    ThreadRunner writer_thread_;
    std::queue<std::tuple<Ts...>> queue_;
};

template <typename... Ts>
bool AsyncSink<DataSink<Ts...>>::put(Ts &&...obj) {
    std::lock_guard<std::mutex> locker(lock_);

    if (!writer_thread_.is_running()) {
        return false;
    }

    queue_.emplace(std::move(obj)...);
    available_.notify_one();
    return true;
}

template <typename... Ts>
bool AsyncSink<DataSink<Ts...>>::write() {
    std::unique_lock<std::mutex> locker(lock_);

    available_.wait(locker, [this]() {
        return !writer_thread_.is_running() || !queue_.empty();
    });

    if (!queue_.empty()) {
        auto data = std::move(queue_.front());
        queue_.pop();

        locker.unlock();
        auto fn_put = [this](Ts &&... args) {
            return sink_->put(std::move(args)...);
        };
        return Invoke<bool, decltype(fn_put), std::tuple<Ts...>>::template invoke<>(std::move(fn_put), std::move(data));
    }

    return false;
}

template <typename... Ts>
void AsyncSink<DataSink<Ts...>>::close() {
    writer_thread_.stop();
    sink_->close();
}

template <typename T>
std::shared_ptr<typename T::sink_type> make_async_sink(std::shared_ptr<T> &&sink) {
    return std::make_shared<AsyncSink<typename T::sink_type>>(std::move(sink));
}
