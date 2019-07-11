#include "face_quality_checker.hpp"
#include <vector>
#include <opencv2/opencv.hpp>

namespace face {
    namespace quality {

        bool FaceResolutionCheck::check(const cv::Mat &full_image, const cv::Mat &face_image,
                                        const face_info_t &info) {
            auto &bbox = info.bbox;

            return ((bbox.x2 - bbox.x1 + 1 >= width) &&
                    (bbox.y2 - bbox.y1 + 1 >= height));
        }

        bool FaceBoundaryCheck::check(const cv::Mat &full_image, const cv::Mat &face_image,
                                      const face_info_t &info) {
            cv::Mat mat = face_image;
            auto &bbox = info.bbox;

            return ((bbox.x1 >= 0 && bbox.x2 < mat.size[1]) &&
                    (bbox.y1 >= 0 && bbox.y2 < mat.size[0]));
        }

        bool LandmarkBoundaryCheck::check(const cv::Mat &full_image, const cv::Mat &face_image,
                                          const face_info_t &info) {
            for (auto x : info.face_pts.x) {
                if (x < info.bbox.x1 || x > info.bbox.x2) {
                    return false;
                }
            }
            for (auto y : info.face_pts.y) {
                if (y < info.bbox.y1 || y > info.bbox.y2) {
                    return false;
                }
            }
            return true;
        }


        bool SkinAreaCheck::check(const cv::Mat &full_image, const cv::Mat &face_image,
                                  const face_info_t &info) {
            cv::Mat hsv_img;
            size_t skin_pixel = 0;

            try {
                cv::cvtColor((cv::Mat)face_image, hsv_img, cv::COLOR_BGR2HSV);
            } catch (cv::Exception &e) {
                return false;
            }

            if (hsv_img.size[0] <= 0 || hsv_img.size[1] <= 0) {
                return false;
            }

            for (size_t i = 0; i < hsv_img.size[0]; i++) {
                for (size_t j = 0; j < hsv_img.size[1]; j++) {
                    int hsv = hsv_img.at<int>(i, j);
                    std::array<int, 3> split = {
                        (hsv >> 16) & 0xff,
                        (hsv >> 8) & 0xff,
                        hsv & 0xff
                    };

                    bool bounded = true;
                    for (size_t k = 0; k < 3; k++) {
                        if (hsv_lb[k] >= split[k] || hsv_ub[k] <= split[k]) {
                            bounded = false;
                        }
                    }
                    if (bounded) {
                        skin_pixel++;
                    }
                }
            }

            return ((double)skin_pixel / hsv_img.size[0] / hsv_img.size[1]) > threshold;
        }

        bool FaceClarityCheck::check(const cv::Mat &full_image, const cv::Mat &face_image,
                                     const face_info_t &info) {
            cv::Mat gray;
            cv::cvtColor((cv::Mat)face_image, gray, cv::COLOR_BGR2GRAY);
            cv::Laplacian(gray, gray, CV_8U);
            cv::Mat mu, sigma;
            cv::meanStdDev(gray, mu, sigma);
            double variance_sqrt = sigma.at<double>(0, 0);

            return variance_threshold < variance_sqrt * variance_sqrt;
        }

        bool FaceposeCheck::check(const cv::Mat &full_image, const cv::Mat &face_image,
                                  const face_info_t &info) {
            std::vector<std::vector<float>> detect_result;

            try {
                detector_->Detect({(cv::Mat)face_image}, detect_result);
            } catch (cv::Exception &e) {
                throw std::runtime_error(std::string("cv::Exception: ") + e.what());
            }

            const size_t feature_offset = 136;
            const size_t feature_count = 3;

            for (size_t i = 0; i < feature_count; i++) {
                if (abs(detect_result[0][feature_offset + i] * 90) > threshold_) {
                    return false;
                }
            }

            return true;
        }

        bool AntiSpoofingCheck::check(const cv::Mat &full_image, const cv::Mat &face_image,
                                      const face_info_t &info) {
            std::vector<cv::Rect> face_rect;
            float x = info.bbox.x1;
            float y = info.bbox.y1;
            float w = info.bbox.x2 - info.bbox.x1 + 1;
            float h = info.bbox.y2 - info.bbox.y1 + 1;
            face_rect.push_back(cv::Rect(x, y, w, h));

            std::vector<bool> passing;
            anti_spoofing_->Detect(full_image, face_rect, passing);

            return passing[0];
        }
    }
}