#pragma once

#include "pipe_component.hpp"
#include "repository.hpp"
#include "image.hpp"
#include "face_api.hpp"
#include <vector>
#include <memory>
#include <mutex>
#include "mp/tuple.hpp"
#include "function_tracer.h"
#include "fdfr.hpp"
#include "json.hpp"
#include "memory/bytes.hpp"

namespace face {
    namespace pipeline {
        class FD : public ::pipeline::Component<image_t &> {
        public:
            explicit FD(const json &config);
            FD(face_algorithm_t algo, const std::vector<std::string> &models);

            std::tuple<std::vector<face_info_t>> process(std::tuple<image_t &> tuple);

        private:
            std::mutex m;
            std::unique_ptr<::FD> fd_;
        };

        class FR : public ::pipeline::Component<image_t &, std::vector<face_info_t> &> {
        public:
            explicit FR(const json &config);
            FR(face_algorithm_t algo, const std::vector<std::string> &models);

            std::tuple<std::vector<qnn::vision::FaceAttributeInfo>> process(std::tuple<image_t &, std::vector<face_info_t> &> tuple);

        private:
            std::mutex m;
            std::unique_ptr<::FR> fr_;
        };
    }
}
