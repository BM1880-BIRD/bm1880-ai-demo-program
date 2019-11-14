#pragma once

#include "image.hpp"
#include "pipeline_core.hpp"
#include "qnn.hpp"
#include "mp/tuple.hpp"

namespace pose {
    namespace pipeline {
        class PoseDetection : public ::pipeline::Component<image_t &> {
        public:
            explicit PoseDetection(const json &config) :
            PoseDetection(config.at("model_path").get<std::string>(), config.at("multi_detect").get<int>()) {}

            PoseDetection(std::string &&model_path, int multi_detect = 1) {
                detector_.reset(new qnn::vision::Openpose(model_path, "COCO", multi_detect));
            }

            std::tuple<> process(std::tuple<image_t &> tuple) {
                cv::Mat image = std::get<0>(tuple);
                std::vector<std::vector<cv::Point2f>> results;
                detector_->Detect(image, results);
                detector_->Draw(results, image);

                return std::make_tuple();
            }

        private:
            std::unique_ptr<qnn::vision::Openpose> detector_;
        };
    }
}
