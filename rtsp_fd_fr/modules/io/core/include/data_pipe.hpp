#pragma once

#include "data_sink.hpp"
#include "data_source.hpp"
#include <memory>
#include <mutex>
#include <queue>
#include <condition_variable>

enum class data_pipe_mode {
    blocking,
    dropping,
};

template <typename... Ts>
class DataPipe : public DataSink<Ts...>, public DataSource<Ts...> {
public:

    DataPipe(int capacity, data_pipe_mode mode) :
    DataSink<Ts...>(typeid(*this).name()),
    DataSource<Ts...>(typeid(*this).name()),
    capacity_(capacity), opened(true), mode_(mode) {}

    bool put(Ts &&...obj) override {
        std::unique_lock<std::mutex> locker(mutex_);

        if (mode_ == data_pipe_mode::dropping && capacity_ > 0 && data_.size() >= capacity_) {
            return opened;
        }

        space_avail_.wait(locker, [this]() {
            return !opened || capacity_ <= 0 || data_.size() < capacity_;
        });

        if (!opened) {
            return false;
        }

        data_.emplace(std::move(obj)...);
        data_avail_.notify_one();

        return true;
    }

    void close() override {
        std::lock_guard<std::mutex> locker(mutex_);
        opened = false;

        space_avail_.notify_all();
        data_avail_.notify_all();
    }

    bool poll(std::chrono::microseconds timeout = std::chrono::microseconds(0)) {
        std::unique_lock<std::mutex> locker(mutex_);
        data_avail_.wait_for(locker, timeout, [this]() {
            return (!opened) || !data_.empty();
        });

        return !data_.empty();
    }

    mp::optional<Ts...> try_get(std::chrono::microseconds timeout = std::chrono::microseconds(0)) {
        std::unique_lock<std::mutex> locker(mutex_);
        data_avail_.wait_for(locker, timeout, [this]() {
            return (!opened) || !data_.empty();
        });

        mp::optional<Ts...> result;

        if (!data_.empty()) {
            result.set(std::move(data_.front()));
            data_.pop();
        }
        space_avail_.notify_one();

        return result;
    }

    mp::optional<Ts...> get() override {
        std::unique_lock<std::mutex> locker(mutex_);
        data_avail_.wait(locker, [this]() {
            return (!opened) || !data_.empty();
        });

        mp::optional<Ts...> result;

        if (!data_.empty()) {
            result.set(std::move(data_.front()));
            data_.pop();
        }
        space_avail_.notify_one();

        return result;
    }

    bool is_opened() override {
        std::lock_guard<std::mutex> locker(mutex_);
        return opened;
    }

    void clear() {
        std::lock_guard<std::mutex> locker(mutex_);
        data_ = std::queue<std::tuple<Ts...>>();
        space_avail_.notify_all();
    }

private:
    data_pipe_mode mode_;
    int capacity_;
    std::mutex mutex_;
    std::condition_variable data_avail_;
    std::condition_variable space_avail_;
    bool opened;
    std::queue<std::tuple<Ts...>> data_;
};

template <typename Tuple>
class TupleDataPipe;

template <typename... Ts>
class TupleDataPipe<std::tuple<Ts...>> : public DataPipe<typename std::decay<Ts>::type...> {
public:
    using DataPipe<typename std::decay<Ts>::type...>::DataPipe;
};