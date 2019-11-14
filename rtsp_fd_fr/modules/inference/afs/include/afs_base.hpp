#pragma once

#include "pipeline_core.hpp"
#include "json.hpp"
#include "mp/strong_typedef.hpp"
#include "vector_pack.hpp"
#include "image.hpp"
#include "afs_utils.hpp"
#include <memory>

namespace inference {
namespace pipeline {

class AFSBase : public ::pipeline::Component<image_t &, VectorPack &> {
public:
    explicit AFSBase(const json &config);
    std::tuple<> process(std::tuple<image_t &, VectorPack &> tuple);

protected:
    cv::Rect squareRectScale(const cv::Rect &rect, const float scale_val);
    cv::Mat cropImage(const cv::Mat &image, const cv::Rect &face_rect);
    cv::Rect RectScale(const cv::Rect &rect, const int cols, const int rows,
                       const float scale_val, const int offset_x = 0, const int offset_y = 0);

    virtual result_item_t getItem(std::tuple<image_t &, VectorPack &> tuple) = 0;
};

}
}