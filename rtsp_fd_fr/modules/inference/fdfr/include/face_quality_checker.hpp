#pragma once

#include "face_quality_check.hpp"
#include "qnn.hpp"

namespace face {
    namespace quality {

        class FaceResolutionCheck : public QualityChecker {
        public:
            FaceResolutionCheck(size_t width, size_t height) : QualityChecker(FaceResolution), width(width), height(height) {}
            FaceResolutionCheck(size_t size) : FaceResolutionCheck(size, size) {}

            bool check(const cv::Mat &full_image, const cv::Mat &face_image,
                       const face_info_t &info) override;

        private:
            size_t width;
            size_t height;
        };

        class FaceBoundaryCheck : public QualityChecker {
        public:
            FaceBoundaryCheck() : QualityChecker(FaceBoundary) {}

            bool check(const cv::Mat &full_image, const cv::Mat &face_image,
                       const face_info_t &info) override;
        };

        class LandmarkBoundaryCheck : public QualityChecker {
        public:
            LandmarkBoundaryCheck() : QualityChecker(LandmarkBoundary) {}

            bool check(const cv::Mat &full_image, const cv::Mat &face_image,
                       const face_info_t &info) override;
        };

        class SkinAreaCheck : public QualityChecker {
        public:
            SkinAreaCheck(double ratio_threshold) :
            QualityChecker(SkinArea), threshold(ratio_threshold), hsv_lb({0, 0, 0}), hsv_ub({255, 255, 255}) {}
            SkinAreaCheck(double ratio_threshold, const std::array<int, 3> &lb, const std::array<int, 3> &ub) :
            QualityChecker(SkinArea), threshold(ratio_threshold), hsv_lb(lb), hsv_ub(ub) {}

            bool check(const cv::Mat &full_image, const cv::Mat &face_image,
                       const face_info_t &info) override;

        private:
            double threshold;
            std::array<int, 3> hsv_lb;
            std::array<int, 3> hsv_ub;
        };

        class FaceClarityCheck : public QualityChecker {
        public:
            FaceClarityCheck(double variance_threshold) : QualityChecker(FaceClarity), variance_threshold(variance_threshold) {}

            bool check(const cv::Mat &full_image, const cv::Mat &face_image,
                       const face_info_t &info) override;

        private:
            double variance_threshold;
        };

        class FaceposeCheck : public QualityChecker {
        public:
            FaceposeCheck(double threshold, const std::shared_ptr<qnn::vision::Facepose> &detector) :
            QualityChecker(FacePose), threshold_(threshold), detector_(detector) {}

            bool check(const cv::Mat &full_image, const cv::Mat &face_image,
                       const face_info_t &info) override;

        private:
            double threshold_;
            std::shared_ptr<qnn::vision::Facepose> detector_;
        };

        class AntiSpoofingCheck : public QualityChecker {
        public:
            AntiSpoofingCheck(std::shared_ptr<qnn::vision::AntiFaceSpoofing> anti_spoofing) :
            QualityChecker(AntiSpoofing), anti_spoofing_(anti_spoofing) {}

            bool check(const cv::Mat &full_image, const cv::Mat &face_image,
                       const face_info_t &info) override;

        private:
            std::shared_ptr<qnn::vision::AntiFaceSpoofing> anti_spoofing_;
        };

    }
}