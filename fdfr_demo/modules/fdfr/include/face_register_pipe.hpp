#pragma once

#include <tuple>
#include <vector>
#include "pipeline_core.hpp"
#include "fdfr.hpp"
#include "repository.hpp"
#include "function_tracer.h"
#include "tuple.hpp"
#include "face_quality_checker.hpp"
#include "vector_pack.hpp"

namespace face {
    namespace pipeline {

        class RegisterFace : public ::pipeline::Component<const std::string &, const face_features_t &> {
        public:
            RegisterFace(std::shared_ptr<Repository> repo) : repo_(std::move(repo)) {}

            std::tuple<> process(std::tuple<const std::string &, const face_features_t &> args) {
                BITMAIN_FUNCTION_TRACE(__PRETTY_FUNCTION__);
                repo_->add(std::get<0>(args), std::get<1>(args).face_feature.features);

                return {};
            }

        private:
            std::shared_ptr<Repository> repo_;
        };

        class RecognizeFace : public ::pipeline::Component<std::vector<face_info_t> &, const std::vector<face_features_t> &> {
        public:
            RecognizeFace(double threshold, std::shared_ptr<Repository> repo) : threshold_(threshold), repo_(std::move(repo)) {}

            std::tuple<std::vector<fr_name_t>> process(std::tuple<std::vector<face_info_t> &, const std::vector<face_features_t> &> tuple) {
                BITMAIN_FUNCTION_TRACE(__PRETTY_FUNCTION__);
                auto &face_infos = std::get<0>(tuple);
                auto &features_list = std::get<1>(tuple);

                std::vector<fr_name_t> names(face_infos.size(), {{}});
                int name_size = sizeof(fr_name_t);
                for (size_t i = 0; i < face_infos.size(); i++) {
                    auto result = repo_->match(features_list[i].face_feature.features, threshold_);
                    memset(names[i].data(), 0, name_size);
                    if (!result.matched) {
                        strncpy(names[i].data(), "UNKNOWN", name_size - 1);
                    } else {
                        strncpy(names[i].data(), repo_->get_match_name(result).data(), name_size - 1);
                    }
                    face_infos[i].bbox.score = result.score;
                }
                return std::make_tuple(std::move(names));
            }

        private:
            const double threshold_;
            std::shared_ptr<Repository> repo_;
        };

        class IdentifyFace : public ::pipeline::Component<VectorPack &> {
        public:
            explicit IdentifyFace(const json &config) :
            threshold_(config.at("similarity_threshold").get<double>()),
            repo_(std::make_shared<Repository>(config.at("repository").get<std::string>())) {}

            IdentifyFace(double threshold, std::shared_ptr<Repository> repo) : threshold_(threshold), repo_(std::move(repo)) {}

            std::tuple<> process(std::tuple<VectorPack &> tuple) {
                BITMAIN_FUNCTION_TRACE(__PRETTY_FUNCTION__);
                auto &vp = std::get<0>(tuple);
                const size_t face_count = std::min(vp.size<face_info_t>(), vp.size<face_features_t>());
                std::vector<fr_name_t> names(face_count);

                for (size_t i = 0; i < face_count; i++) {
                    auto result = repo_->match(vp.at<face_features_t>(i).face_feature.features, threshold_);
                    memset(names[i].data(), 0, names[i].size());
                    if (!result.matched) {
                        strncpy(names[i].data(), "UNKNOWN", names[i].size() - 1);
                    } else {
                        strncpy(names[i].data(), repo_->get_match_name(result).data(), names[i].size() - 1);
                    }
                    vp.at<face_info_t>(i).bbox.score = result.score;
                }

                return {};
            }

        private:
            const double threshold_;
            std::shared_ptr<Repository> repo_;
        };

        class RegisterFaceCheck : public ::pipeline::Component<const std::string &, const image_t &, const std::vector<face_info_t> &, const std::vector<face_features_t> &> {
        public:
            RegisterFaceCheck(std::shared_ptr<RegisterFace> registerer, std::vector<std::shared_ptr<face::quality::QualityChecker>> &&checkers) :
            registerer(registerer),
            checkers(std::move(checkers)) {}

            std::tuple<bool, uint64_t> process(std::tuple<const std::string &, const image_t &, const std::vector<face_info_t> &, const std::vector<face_features_t> &> tuple) {
                auto &faces = mp::tuple_get<const std::vector<face_info_t> &>(tuple);
                auto &features = mp::tuple_get<const std::vector<face_features_t> &>(tuple);

                if (1 != faces.size() || 1 != features.size()) {
                    return std::make_tuple<bool, uint64_t>(false, 1 << quality::FaceCount);
                }

                auto face = faces.front();
                face_features_t feature = std::move(features.front());
                cv::Mat full_image = mp::tuple_get<const image_t &>(tuple);
                cv::Mat face_image;

                try {
                    face_image = full_image(cv::Range(std::max<int>(0, face.bbox.y1), std::min<int>(face.bbox.y2, full_image.size[0] - 1)),
                                            cv::Range(std::max<int>(0, face.bbox.x1), std::min<int>(face.bbox.x2, full_image.size[1] - 1)));
                } catch (cv::Exception &e) {
                    throw std::runtime_error("cv::Exception: " + std::string(e.what()));
                }

                uint64_t error_flags = 0;
                for (auto &checker : checkers) {
                    if (!checker->check(mp::tuple_get<const image_t &>(tuple), face_image, face)) {
                        error_flags |= (0x1 << checker->code());
                    }
                }

                if (error_flags == 0) {
                    registerer->process(std::make_tuple(mp::tuple_get<const std::string &>(tuple), feature));
                }

                return std::make_tuple(error_flags == 0, error_flags);
            }

            void set_context(std::shared_ptr<::pipeline::Context> context) {
                registerer->set_context(context);
            }

        private:
            std::shared_ptr<RegisterFace> registerer;
            std::vector<std::shared_ptr<face::quality::QualityChecker>> checkers;
        };
    }
}
