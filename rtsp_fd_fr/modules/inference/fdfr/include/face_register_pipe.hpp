#pragma once

#include <tuple>
#include <vector>
#include "pipeline_core.hpp"
#include "fdfr.hpp"
#include "repository.hpp"
#include "function_tracer.h"
#include "mp/tuple.hpp"
#include "face_quality_checker.hpp"
#include "vector_pack.hpp"
#include "memory/bytes.hpp"
#include "logging.hpp"
#include "mp/strong_typedef.hpp"

namespace face {
    namespace pipeline {
        class Register : public ::pipeline::Component<const std::string &, const std::vector<face_features_t> &> {
        public:
            MP_STRONG_TYPEDEF(bool, result_t);

            explicit Register(const json &config) : repo_(std::make_shared<Repository>(config.at("repository").get<std::string>())) {}
            explicit Register(std::shared_ptr<Repository> repo) : repo_(std::move(repo)) {}

            std::tuple<result_t> process(std::tuple<const std::string &, const std::vector<face_features_t> &> args) {
                BITMAIN_FUNCTION_TRACE(__PRETTY_FUNCTION__);

                if (std::get<1>(args).size() == 1) {
                    repo_->add(std::get<0>(args), std::get<1>(args)[0].face_feature.features);
                    return std::make_tuple<result_t>(true);
                } else {
                    LOGI << "input image of " << std::get<0>(args) << " has more than 1 face";
                    return std::make_tuple<result_t>(false);
                }
            }

        private:
            std::shared_ptr<Repository> repo_;
        };

        static inline void to_json(json &j, const Register::result_t &v) {
            j = bool(v);
        }

        class RegisterNNM : public ::pipeline::Component<std::vector<face_info_t> &, std::vector<face_features_t> &> {
        public:
            explicit RegisterNNM(const json &config, const std::string name) :
            name_(name),
            repo_(std::make_shared<Repository>(config.at("repository").get<std::string>())) {}

            RegisterNNM(std::string name, std::shared_ptr<Repository> repo) : name_(name), repo_(std::move(repo)) {}

            std::tuple<std::vector<fr_name_t>> process(std::tuple<std::vector<face_info_t> &, const std::vector<face_features_t> &> tuple) {
                BITMAIN_FUNCTION_TRACE(__PRETTY_FUNCTION__);
                auto &feature_list = std::get<1>(tuple);
                std::vector<fr_name_t> names(feature_list.size());

                if (feature_list.size() == 1) {
                    static int count = 0;

                    if (count < register_count_) {
                        repo_->add(name_, feature_list[0].face_feature.features);
                        ++count;
                    } else {
                        exit(0);
                    }
                } else {
                    LOGI << "input image of has more than 1 face";
                }

                return std::make_tuple(std::move(names));
            }

        private:
            int register_count_ = 1;
            std::string name_;
            std::shared_ptr<Repository> repo_;
        };

        class Identify : public ::pipeline::Component<std::vector<face_info_t> &, std::vector<face_features_t> &> {
        public:
            explicit Identify(const json &config) :
            threshold_(config.at("similarity_threshold").get<double>()),
            repo_(std::make_shared<Repository>(config.at("repository").get<std::string>())) {}

            Identify(double threshold, std::shared_ptr<Repository> repo) : threshold_(threshold), repo_(std::move(repo)) {}

            std::tuple<std::vector<fr_name_t>> process(std::tuple<std::vector<face_info_t> &, const std::vector<face_features_t> &> tuple) {
                BITMAIN_FUNCTION_TRACE(__PRETTY_FUNCTION__);
                auto &face_infos = std::get<0>(tuple);
                auto &feature_list = std::get<1>(tuple);
                std::vector<fr_name_t> names(feature_list.size());

                for (size_t i = 0; i < feature_list.size(); i++) {
                    auto result = repo_->match(feature_list[i].face_feature.features, threshold_);
                    memset(names[i].data(), 0, sizeof(fr_name_t));
                    if (!result.matched) {
                        strncpy(names[i].data(), "UNKNOWN", sizeof(fr_name_t));
                    } else {
                        strncpy(names[i].data(), repo_->get_match_name(result).data(), sizeof(fr_name_t));
                    }
                    face_infos[i].bbox.score = result.score;
                }

                return std::make_tuple(std::move(names));
            }

        private:
            double threshold_;
            std::shared_ptr<Repository> repo_;
        };
    }
}
