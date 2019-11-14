#pragma once

#include "pipe_component.hpp"
#include "image.hpp"
#include "mp/tuple.hpp"
#include "function_tracer.h"

namespace pipeline {
    enum class IMAGE_CONCAT_DIRECTION {
        HORIZONTAL,
        VERTICAL,
    };

    class ImageConcat : public ::pipeline::Component<const vector<cv::Mat> &> {
    public:

        ImageConcat(IMAGE_CONCAT_DIRECTION direction = IMAGE_CONCAT_DIRECTION::VERTICAL) : direction_(direction) {}

        std::tuple<image_t> process(std::tuple<const vector<cv::Mat> &> tuple) {
            image_t result_image;
            auto &images = std::get<0>(tuple);

            for (auto &image : images) {
                if (result_image.empty()) {
                    result_image = image;
                } else {
                    // Resize matrix and concat
                    cv::Mat temp_mat = result_image;
                    cv::Mat temp_resize_mat;

                    if (IMAGE_CONCAT_DIRECTION::VERTICAL == direction_) {
                        cv::resize(image, temp_resize_mat, cv::Size(temp_mat.cols, image.rows));
                        cv::vconcat(temp_mat, temp_resize_mat, temp_mat);
                    } else {
                        cv::resize(image, temp_resize_mat, cv::Size(image.cols, temp_mat.rows));
                        cv::hconcat(temp_mat, temp_resize_mat, temp_mat);
                    }

                    result_image = temp_mat;
                }
            }

            return std::make_tuple(std::move(result_image));
        }

    private:
        IMAGE_CONCAT_DIRECTION direction_;
    };
}
