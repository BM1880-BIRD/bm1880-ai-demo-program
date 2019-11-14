#pragma once

#include "pipeline_core.hpp"
#include "image.hpp"
#include <memory>
#include <string>
#include <opencv2/opencv.hpp>

namespace object_detection {
    namespace pipeline {

        class Visualize : public ::pipeline::Component<image_t &, const std::vector<qnn::vision::object_detect_rect_t> &> {
        public:
            std::tuple<> process(std::tuple<image_t &, const std::vector<qnn::vision::object_detect_rect_t> &> tuple) {
                cv::Mat image = std::get<0>(tuple);
                auto white = cv::Scalar(255, 255, 255);
                auto black = cv::Scalar(0, 0, 0);
                for (auto &rect : std::get<1>(tuple)) {
                    auto color = cv::Scalar(85 * ((rect.classes / 16) % 4), 85 * ((rect.classes / 4) % 4), 85 * (rect.classes % 4));
                    auto text_color = color[0] + color[1] + color[2] > 255 * 3 / 2 ? black : white;
                    float x = rect.x1;
                    float y = rect.y1;
                    float w = rect.x2 - rect.x1 + 1;
                    float h = rect.y2 - rect.y1 + 1;
                    cv::rectangle(image, cv::Rect(x, y, w, h), color, 2);
                    int baseline;
                    auto text_size = cv::getTextSize(rect.label, 0, 1, 1, &baseline);
                    cv::rectangle(image, cv::Rect(x, y - text_size.height, text_size.width, text_size.height), color, -1);
                    cv::putText(image, rect.label, cv::Point(x, y), 0, 1, text_color);
                }
            }
        };

    }
}
