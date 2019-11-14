#pragma once

#include <tuple>
#include <memory>
#include "face_api.hpp"
#include "pipeline_core.hpp"
#include "lru_cache.hpp"
#include "repository.hpp"
#include "volatile_repo.hpp"
#include "qnn.hpp"
#include "repository.hpp"
#include "json.hpp"

namespace face {
namespace pipeline {

class FaceAttrStabilizer : public ::pipeline::Component<image_t &, std::vector<face_features_t> &> {
    using StabilizeMethod = qnn::vision::StabilizeMethod;
public:
    explicit FaceAttrStabilizer(const json &config);
    std::tuple<> process(std::tuple<image_t &, std::vector<face_features_t> &> tuple);

private:
    qnn::vision::FaceAttributeStabilizer<std::pair<bool, size_t>> stablizer;
    std::shared_ptr<Repository> repo;
    double similarity_threshold;
    LRUCache<size_t> strangers_lru;
    double stranger_similarity_threshold;
    VolatileRepo<FACE_FEATURES_DIM> stranger_repo;
};

}
}