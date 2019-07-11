#pragma once

#include <vector>
#include "pipe_component.hpp"
#include "image.hpp"
#include "face_api.hpp"
#include "tuple.hpp"
#include "function_tracer.h"
#include "qnn.hpp"

namespace face {
    namespace pipeline {
        class FaceAntiSpoofing : public ::pipeline::Component<const image_t &, const std::vector<face_info_t> &> {
        public:
            FaceAntiSpoofing(std::vector<std::string> &&models, bool draw_label = false) : draw_label_(std::move(draw_label)) {
                if (models.size() == 5) {
                    anti_spoofing_.reset(new qnn::vision::AntiFaceSpoofing(models[0], models[1], models[2], models[3], models[4], models[5]));
                }
            }

            std::tuple<std::vector<bool>, cv::Mat> process(std::tuple<const image_t &, const std::vector<face_info_t> &> tuple) {
                BITMAIN_FUNCTION_TRACE(__PRETTY_FUNCTION__);
                auto &image = std::get<0>(tuple);
                auto face_infos = std::get<1>(tuple);
                std::vector<bool> is_real_faces;
                cv::Mat depth_face_mat;

                if ((face_infos.size() > 0) && (nullptr != anti_spoofing_)) {
                    std::sort(face_infos.begin(), face_infos.end(), [](face_info_t &a, face_info_t &b) {
                        return (a.bbox.x2 - a.bbox.x1 + 1) * (a.bbox.y2 - a.bbox.y1 + 1) >
                            (b.bbox.x2 - b.bbox.x1 + 1) * (b.bbox.y2 - b.bbox.y1 + 1);
                    });

                    // Only detect the largest face
                    std::vector<cv::Rect> face_rect;
                    for (size_t i = 0; i < 1 && i < face_infos.size(); i++) {
                        float x = face_infos[i].bbox.x1;
                        float y = face_infos[i].bbox.y1;
                        float w = face_infos[i].bbox.x2 - face_infos[i].bbox.x1 + 1;
                        float h = face_infos[i].bbox.y2 - face_infos[i].bbox.y1 + 1;
                        face_rect.push_back(cv::Rect(x, y, w, h));
                    }
                    anti_spoofing_->Detect(image, face_rect, is_real_faces);
                    // Get face depth matrix
                    depth_face_mat = anti_spoofing_->GetDepthVisualMat();

                    if (draw_label_) {
                        for (size_t i = 0; i < 1 && i < is_real_faces.size(); i++) {
                            cv::Mat img = (cv::Mat)image;
                            cv::String cv_s = is_real_faces[i] ? "Real" : "Fake";
                            cv::putText(img, cv_s, cv::Point(face_rect[i].x, face_rect[i].y + 30), 0, 1, cv::Scalar(0, 0, 255), 3);
                        }
                    }
                }

                return std::make_tuple(std::move(is_real_faces), std::move(depth_face_mat));
            }

        private:
            bool draw_label_;
            std::unique_ptr<qnn::vision::AntiFaceSpoofing> anti_spoofing_;
        };
    }
}
