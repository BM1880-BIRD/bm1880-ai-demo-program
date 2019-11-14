#pragma once

#include "pipeline_core.hpp"
#include "json.hpp"
#include "mp/strong_typedef.hpp"
#include "vector_pack.hpp"
#include "image.hpp"
#include <face/anti_facespoofing/afs_canny.hpp>
#include <memory>
#include <deque>

namespace inference {
namespace pipeline {

class AFSResult : public ::pipeline::Component<image_t &, VectorPack &> {
public:
    struct result_t {
        bool liveness;
        bool weighted_liveness;
        bool vote_liveness;
        float weighted_confidence;
        float weighted_threshold;
    };

public:
    explicit AFSResult(const json &config);
    std::tuple<> process(std::tuple<image_t &, VectorPack &> tuple);

private:
    void UpdateVoteQueue(cv::Rect rect, bool liveness);
    bool GetVoteResult();

private:
    float weighted_threshold = 0;
    float vote_threshold = 0.6;
    float vote_iou_threshold = 0.7;
    int vote_size = 10;
    cv::Rect vote_rect;
    std::deque<bool> vote_queue;
};

inline void to_json(json &j, const AFSResult::result_t &t) {
    j = json{{"liveness", t.liveness},
             {"weighted_liveness", t.weighted_liveness},
             {"vote_liveness", t.vote_liveness},
             {"weighted_confidence", t.weighted_confidence},
             {"weighted_threshold", t.weighted_threshold}};
}
inline void from_json(const json &j, AFSResult::result_t &t) {
    t.liveness = j.at("liveness").get<bool>();
    t.weighted_liveness = j.at("weighted_liveness").get<bool>();
    t.vote_liveness = j.at("vote_liveness").get<bool>();
    t.weighted_confidence = j.at("weighted_confidence").get<float>();
    t.weighted_threshold = j.at("weighted_threshold").get<float>();
}

}
}

REGISTER_UNIQUE_TYPE_NAME(inference::pipeline::AFSResult::result_t);