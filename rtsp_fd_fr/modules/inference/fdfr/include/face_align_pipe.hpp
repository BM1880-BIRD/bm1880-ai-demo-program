#pragma once

#include <vector>
#include <tuple>
#include <sstream>
#include "image.hpp"
#include "pipeline_core.hpp"
#include "qnn.hpp"
#include "mp/tuple.hpp"

namespace face {
    namespace pipeline {

        class FaceAlign : public ::pipeline::Component<const image_t &, const std::vector<face_info_t> &> {
        public:
            FaceAlign(size_t width) : width_(width) {}

            std::tuple<std::vector<image_t>> process(std::tuple<const image_t &, const std::vector<face_info_t> &> tuple) {
                auto &image = mp::tuple_get<const image_t &>(tuple);
                auto &faces = mp::tuple_get<const std::vector<face_info_t> &>(tuple);
                std::vector<image_t> aligned_images;
                cv::Mat img = (cv::Mat)image;

                for (auto &face : faces) {
                    cv::Mat aligned_image(width_, width_, img.type());
                    face_align(img, aligned_image, face, width_, width_);
                    aligned_images.emplace_back(std::move(aligned_image));
                }

                return std::make_tuple(std::move(aligned_images));
            }

        private:
            size_t width_;
        };

        template <typename T>
        class FaceCrop : public ::pipeline::Component<const image_t &, const std::vector<face_info_t> &> {
        public:
            std::tuple<std::vector<T>> process(std::tuple<const image_t &, const std::vector<face_info_t> &> tuple) {
                auto &image = mp::tuple_get<const image_t &>(tuple);
                auto &faces = mp::tuple_get<const std::vector<face_info_t> &>(tuple);
                std::vector<T> aligned_images;
                cv::Mat img = (cv::Mat)image;

                for (auto &face : faces) {
                    try {
                        cv::Mat aligned_image = img(cv::Range(std::max<int>(0, face.bbox.y1), std::min<int>(face.bbox.y2, img.size[0] - 1)),
                                                    cv::Range(std::max<int>(0, face.bbox.x1), std::min<int>(face.bbox.x2, img.size[1] - 1)));
                        aligned_images.emplace_back(aligned_image);
                    } catch (cv::Exception &e) {
                        throw std::runtime_error(std::string("cv::Exception: ") + e.what());
                    }
                }

                return std::make_tuple(std::move(aligned_images));
            }
        };

    }
}
