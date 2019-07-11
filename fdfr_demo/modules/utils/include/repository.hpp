#pragma once

#include "face_api.hpp"
#include "cached_repo.hpp"
#include "annoy_repo.hpp"
#include "tpu_repo.hpp"

// typedef AnnoyRepo<float, 512> Repository;
// typedef CachedRepo<512> Repository;
typedef TpuRepo<FACE_FEATURES_DIM> Repository;
