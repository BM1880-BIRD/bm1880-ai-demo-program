#pragma once

#include <memory>
#include <mutex>
#include <queue>
#include <condition_variable>
#include "data_source.hpp"
#include "thread_runner.hpp"

template <typename SourceType>
class AsyncSource;

template <typename... Ts>
class AsyncSource<DataSource<Ts...>> : public DataSource<Ts...> {
public:
    AsyncSource(std::shared_ptr<DataSource<Ts...>> &&src, size_t capacity = 0) : src_(std::move(src)), capacity_(capacity) {
        reader_thread_.start([this]() {
            read();
        });
    }
    ~AsyncSource() {
        reader_thread_.stop();
        available_.notify_all();
    }

    mp::optional<Ts...> get() override;
    bool is_opened() override {
        return src_->is_opened();
    }
    void close() override {
        src_->close();
    }

private:
    void read();

    std::shared_ptr<DataSource<Ts...>> src_;
    size_t capacity_;
    std::mutex lock_;
    std::condition_variable available_;
    ThreadRunner reader_thread_;
    std::queue<mp::optional<Ts...>> queue_;
};

template <typename... Ts>
mp::optional<Ts...> AsyncSource<DataSource<Ts...>>::get() {
    std::unique_lock<std::mutex> locker(lock_);
    mp::optional<Ts...> result;

    available_.wait(locker, [this]() {
        return !reader_thread_.is_running() || !queue_.empty();
    });

    if (!queue_.empty()) {
        result = std::move(queue_.front());
        queue_.pop();
    }

    return result;
}

template <typename... Ts>
void AsyncSource<DataSource<Ts...>>::read() {
    mp::optional<Ts...> obj = src_->get();

    if (!obj.has_value()) {
        reader_thread_.stop();
    } else {
        std::lock_guard<std::mutex> locker(lock_);
        queue_.emplace(std::move(obj));

        if ((capacity_ > 0) && (queue_.size() > capacity_)) {
            queue_.pop();
        }
    }

    available_.notify_one();
}

template <typename T>
std::shared_ptr<typename T::source_type> make_async_src(std::shared_ptr<T> &&src) {
    return std::make_shared<AsyncSource<typename T::source_type>>(std::move(src));
}

template <typename T>
std::shared_ptr<typename T::source_type> make_async_src(std::shared_ptr<T> &&src, size_t capacity) {
    return std::make_shared<AsyncSource<typename T::source_type>>(std::move(src), capacity);
}
