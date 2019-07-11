#pragma once

#include <memory>
#include <vector>
#include <tuple>
#include <array>
#include <algorithm>
#include "image.hpp"
#include "pipeline_core.hpp"
#include "qnn.hpp"
#include "tuple.hpp"

namespace face {
    namespace pipeline {

        class Facepose : public ::pipeline::Component<const std::vector<cv::Mat> &> {
        public:
            template <typename... Us>
            Facepose(Us &&...args) : detector_(new qnn::vision::Facepose(std::forward<Us>(args)...)) {}

            std::tuple<std::vector<std::array<float, 3>>> process(std::tuple<const std::vector<cv::Mat> &> &&tuple) {
                auto &faces = std::get<0>(tuple);
                std::vector<std::vector<float>> detect_result;
                std::vector<std::array<float, 3>> output(faces.size());

                try {
                    detector_->Detect(faces, detect_result);
                } catch (cv::Exception &e) {
                    throw std::runtime_error(std::string("cv::Exception: ") + e.what());
                }

                const size_t feature_offset = 136;
                for (size_t i = 0; i < detect_result.size(); i++) {
                    for (int j = 0; j < output[i].size() && feature_offset + j < detect_result[i].size(); j++) {
                        output[i][j] = detect_result[i][feature_offset + j] * 90;
                    }
                }

                return std::make_tuple(std::move(output));
            }

        private:
            std::unique_ptr<qnn::vision::Facepose> detector_;
        };

    }
}
