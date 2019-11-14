#pragma once

#include <string>
#include "data_source.hpp"
#include <thread>
#include <chrono>
#include <memory>
#include "json.hpp"
#include "image.hpp"

namespace cv {
    class VideoCapture;
}

class VideoCaptureSource : public DataSource<image_t> {
public:
    explicit VideoCaptureSource(int device);
    explicit VideoCaptureSource(const std::string &filename);
    explicit VideoCaptureSource(const json &config);
    ~VideoCaptureSource();

    mp::optional<image_t> get() override;
    bool is_opened() override;
    void close() override;

private:
    void open();

    json config;
    int retry_grab_count = 0;
    int retry_open_count = 0;
    int retry_grab_sleep = 0;
    int retry_open_sleep = 0;
    std::unique_ptr<cv::VideoCapture> capture;
};