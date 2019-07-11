#pragma once

#include <array>
#include <tuple>
#include <opencv2/opencv.hpp>
#include "face_api.hpp"
#include "pipeline_core.hpp"

namespace face {
    namespace quality {

        enum QualityChecks {
            FaceCount,
            FaceResolution,
            FaceBoundary,
            LandmarkBoundary,
            SkinArea,
            FaceClarity,
            FacePose,
            AntiSpoofing,
        };

        class QualityChecker {
        public:
            QualityChecker(QualityChecks code) : code_(code) {}
            ~QualityChecker() {}

            QualityChecks code() const {
                return code_;
            }

            virtual bool check(const cv::Mat &full_image, const cv::Mat &face_image,
                               const face_info_t &info) = 0;

        private:
            QualityChecks code_;
        };

    }
}

