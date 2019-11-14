#include "capture_stream.hpp"
#include <opencv2/opencv.hpp>

template <typename T>
std::tuple<image_t> CaptureStream<T>::process(std::tuple<>) {
    cv::Mat frame;

    if (source_variable == nullptr) {
        throw std::runtime_error("CaptureStream: stream source is not specified");
    }

    {
        std::lock_guard<std::mutex> lock(source_variable->mutex);
        if (capture == nullptr || !capture->isOpened() || current_source != source_variable->value) {
            current_source = source_variable->value;
            if (capture == nullptr) {
                capture.reset(new cv::VideoCapture(current_source));
            } else {
                capture->open(current_source);
            }
        }
    }

    if (capture != nullptr && !capture->read(frame)) {
        capture->release();
        throw std::runtime_error("CaptureUrlStream: failed to read from VideoCapture");
    }

    return std::make_tuple<image_t>(std::move(frame));
}

template class CaptureStream<int>;
template class CaptureStream<std::string>;
