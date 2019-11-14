#pragma once

#include <tuple>
#include <opencv2/opencv.hpp>
#include <string>
#include "memory/bytes.hpp"
#include "pipe_component.hpp"
#include "image.hpp"
#include "face_api.hpp"
#include "vector_pack.hpp"

namespace cv {
    class Mat;
}

void mark_face(image_t &image, const face_info_t &face_info, const std::string &name, double font_scale);

template <typename U, typename V>
class MarkFace : public pipeline::Component<image_t &&, const U &, const V &> {
public:
    MarkFace(double font_scale = 1) : font_scale_(font_scale) {}

    std::tuple<image_t> process(std::tuple<image_t &&, const U &, const V &> args) {
        auto &img = std::get<0>(args);
        auto &face_infos = std::get<1>(args);
        auto &names = std::get<2>(args);

        for (int i = 0; i < face_infos.size(); i++) {
            mark_face(img, face_infos[i], names.size() == 0 ? "" : (names.begin() + i)->data(), font_scale_);
        }

        return std::make_tuple(std::move(img));
    }

private:
    double font_scale_;
};

class FaceResultVisualizer : public pipeline::Component<image_t &, const VectorPack &> {
public:
    std::tuple<> process(std::tuple<image_t &, const VectorPack &> args);
};