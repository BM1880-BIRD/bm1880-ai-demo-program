#pragma once

#include "data_sink.hpp"
#include <mutex>
#include "image.hpp"

class ImshowSink : public DataSink<image_t> {
public:
    ImshowSink() : DataSink<image_t>("ImshowSink") {
    }

    ImshowSink(std::string &&window_name)  : DataSink<image_t>("ImshowSink"), window_name_(move(window_name)) {}

    bool put(image_t &&frame) override;
    void close() override;
    bool is_opened() override;

private:
    bool closed = false;
    std::string window_name_ = "Output";
    std::mutex lock;
};