#pragma once

#include "pipeline_core.hpp"
#include "json.hpp"
#include "vector_pack.hpp"
#include "image.hpp"
#include "object_detection_encode.hpp"
#include <face/yolo.hpp>
#include <memory>
#include <string>
#include <opencv2/opencv.hpp>

namespace object_detection {
    namespace pipeline {

        class ObjectDetection : public ::pipeline::Component<image_t &> {
        public:
            explicit ObjectDetection(const json &config) {
                auto model_dir = config.at("model_dir").get<std::string>();
                auto model = config.at("model").get<std::string>();
                auto algo = config.at("algorithm").get<std::string>();
                if (algo == "yolov3") {
                    detector_.reset(new qnn::vision::Yolo3(model_dir + "/" + model, "COCO"));
                } else {
                    detector_.reset(new qnn::vision::Yolo2(model_dir + "/" + model, "COCO"));
                }
            }

            std::tuple<std::vector<qnn::vision::object_detect_rect_t>> process(std::tuple<image_t &> tuple) {
                cv::Mat image = std::get<0>(tuple);
                std::vector<qnn::vision::object_detect_rect_t> results;
                detector_->Detect(image, results);
                detector_->Draw(results, image);

                return std::make_tuple(std::move(results));
            }

        private:
            std::unique_ptr<qnn::vision::Yolo2> detector_;
        };

    }
}
