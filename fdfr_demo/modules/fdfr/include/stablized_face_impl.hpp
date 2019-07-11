#pragma once

#include <utility>
#include "face_impl.hpp"
#include "qnn.hpp"
#include "volatile_repo.hpp"
#include "lru_cache.hpp"

class StablizedFaceImpl : public FaceImpl {
    static constexpr size_t unrecognized_repo_capacity = 10;

public:
    StablizedFaceImpl(FDFR &fdfr, Repository &&repo)
        : FaceImpl(fdfr, std::move(repo))
        , strangers_lru(unrecognized_repo_capacity)
        , stablizer(qnn::vision::STABILIZE_METHOD_AVERAGE, 0, 10, 20, 6)
        , stranger_repo() {}
    Memory::SharedView<fr_result_t> faceid(const image_t &image) override;

private:
    LRUCache<size_t> strangers_lru;
    qnn::vision::FaceAttributeStabilizer<std::pair<bool, size_t>> stablizer;
    VolatileRepo<FACE_FEATURES_DIM> stranger_repo;
};
