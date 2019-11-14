#pragma once

#include <memory>
#include "data_sink.hpp"
#include "face_api.hpp"
#include "image.hpp"

template <typename ResultType>
class MarkFaceSink : public DataSink<image_t, ResultType> {
public:
    MarkFaceSink(std::shared_ptr<DataSink<image_t>> &&sink) : DataSink<image_t, ResultType>(typeid(*this).name()), sink_(std::move(sink)) {}

    bool put(image_t &&frame, ResultType &&result) override;
    bool is_opened() override {
        return sink_->is_opened();
    }
    void close() override {
        sink_->close();
    }

protected:
    std::shared_ptr<DataSink<image_t>> sink_;
};

void mark_face(image_t &image, fr_result_t &face);

template <typename ResultType>
bool MarkFaceSink<ResultType>::put(image_t &&frame, ResultType &&result) {
    for (auto &face : result) {
        mark_face(frame, face);
    }
    return sink_->put(std::move(frame));
}
