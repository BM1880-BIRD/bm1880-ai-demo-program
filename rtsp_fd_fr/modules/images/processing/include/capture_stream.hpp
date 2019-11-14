#pragma once

#include <tuple>
#include <string>
#include "pipe_component.hpp"
#include "image.hpp"

namespace cv {
    class VideoCapture;
}

template <typename T>
class CaptureStream : public pipeline::Component<> {
public:
    CaptureStream(std::shared_ptr<pipeline::Context::Variable<T>> variable) : source_variable(variable) {}

    std::tuple<image_t> process(std::tuple<>);

private:
    T current_source;
    std::unique_ptr<cv::VideoCapture> capture;
    std::shared_ptr<pipeline::Context::Variable<T>> source_variable;
};
