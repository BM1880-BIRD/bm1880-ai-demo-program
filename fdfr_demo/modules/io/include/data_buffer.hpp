#pragma once

#include <tuple>
#include <queue>
#include "data_sink.hpp"
#include "data_source.hpp"

template <typename T>
class DataBuffer;

template <typename... Ts>
class DataBuffer<std::tuple<Ts...>> : public DataSink<Ts...>, public DataSource<std::tuple<Ts...>> {
public:
    bool put(Ts &&...input) override {
        data_.emplace(std::move(input)...);
        return true;
    }
    void close() override {
    }

    bool get(std::tuple<Ts...> &output) override {
        if (data_.empty()) {
            return false;
        }
        std::swap(output, data_.front());
        data_.pop();
        return true;
    }

    bool is_opened() {
        return true;
    }

private:
    std::queue<std::tuple<Ts...>> data_;
};
