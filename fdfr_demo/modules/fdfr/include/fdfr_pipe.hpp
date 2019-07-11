#pragma once

#include "pipe_component.hpp"
#include "repository.hpp"
#include "image.hpp"
#include "face_api.hpp"
#include <vector>
#include <memory>
#include <mutex>
#include "tuple.hpp"
#include "function_tracer.h"
#include "fdfr.hpp"
#include "json.hpp"

class VectorPack;

namespace face {
    namespace pipeline {
        class FD : public ::pipeline::Component<image_t &, VectorPack &> {
        public:
            explicit FD(const json &config);

            std::tuple<> process(std::tuple<image_t &, VectorPack &> tuple);

        private:
            std::mutex m;
            std::unique_ptr<::FD> fd_;
        };

        class FR : public ::pipeline::Component<image_t &, VectorPack &> {
        public:
            explicit FR(const json &config);

            std::tuple<> process(std::tuple<image_t &, VectorPack &> tuple);

        private:
            std::mutex m;
            std::unique_ptr<::FR> fr_;
        };
    }
}

class FDPipeComp : public pipeline::Component<const image_t &> {
public:
    FDPipeComp(std::shared_ptr<FDFR> fdfr) : fdfr_(std::move(fdfr)) {}

    std::tuple<std::vector<face_info_t>> process(std::tuple<const image_t &> &&tuple) {
        BITMAIN_FUNCTION_TRACE(__PRETTY_FUNCTION__);
        std::vector<face_info_t> face_infos;
        fdfr_->detect(std::get<0>(tuple), face_infos);
        return std::make_tuple(std::move(face_infos));
    }

private:
    std::shared_ptr<FDFR> fdfr_;
};

class FRPipeComp : public pipeline::Component<const image_t &, const std::vector<face_info_t> &> {
public:
    FRPipeComp(std::shared_ptr<FDFR> fdfr) : fdfr_(std::move(fdfr)) {}

    std::tuple<std::vector<face_features_t>> process(std::tuple<const image_t &, const std::vector<face_info_t> &> tuple) {
        BITMAIN_FUNCTION_TRACE(__PRETTY_FUNCTION__);
        std::vector<face_features_t> features;
        fdfr_->extract(std::get<0>(tuple), std::get<1>(tuple), features);
        return std::make_tuple(std::move(features));
    }

private:
    std::shared_ptr<FDFR> fdfr_;
};

class RegisterFacePipeComp : public pipeline::Component<const std::string &, const image_t &> {
public:
    RegisterFacePipeComp(std::shared_ptr<FDFR> fdfr, std::shared_ptr<Repository> repo) : fdfr_(std::move(fdfr)), repo_(std::move(repo)) {}

    std::tuple<bool> process(std::tuple<const std::string &, const image_t &> args) {
        BITMAIN_FUNCTION_TRACE(__PRETTY_FUNCTION__);
        std::vector<face_info_t> face_infos;
        std::vector<fr_result_t> face_result;
        std::vector<face_features_t> features_list;
        fdfr_->detect_extract(std::get<1>(args), face_infos, features_list);

        if (features_list.size() == 1) {
            repo_->add(std::get<0>(args), features_list[0].face_feature.features);
            return std::make_tuple(true);
        }

        return std::make_tuple(false);
    }

private:
    std::shared_ptr<FDFR> fdfr_;
    std::shared_ptr<Repository> repo_;
};
