#pragma once

#include "mark_face_sink.hpp"

template <typename ResultType>
class MarkFaceAttrSink : public MarkFaceSink<ResultType> {
public:
    MarkFaceAttrSink(std::shared_ptr<DataSink<image_t>> &&sink) : MarkFaceSink<ResultType>(std::move(sink)) {}
    bool put(image_t &&frame, ResultType &&result) override;
};

void mark_attr(image_t &image, fr_result_t &face);

template <typename ResultType>
bool MarkFaceAttrSink<ResultType>::put(image_t &&frame, ResultType &&result) {
    for (auto &face : result) {
        mark_attr(frame, face);
    }
    return this->sink_->put(std::move(frame));
}
