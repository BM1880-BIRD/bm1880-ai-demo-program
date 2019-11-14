#pragma once

#include "pipeline_core.hpp"
#include "json.hpp"
#include "mp/strong_typedef.hpp"
#include "vector_pack.hpp"
#include "image.hpp"
#include <face/anti_facespoofing.hpp>
#include <memory>

namespace inference {
namespace pipeline {

class AntiFaceSpoofing : public ::pipeline::Component<image_t &, VectorPack &> {
public:
    struct result_t {
        bool liveness;
        float classify_confidence;
        float classifyhsv_confidence;
        float wegithed_confidence;
    };

    explicit AntiFaceSpoofing(const json &config);
    std::tuple<> process(std::tuple<image_t &, VectorPack &> tuple);

private:
    bool output_confidence;
    std::unique_ptr<qnn::vision::AntiFaceSpoofing> core;
};

inline void to_json(json &j, const AntiFaceSpoofing::result_t &t) {
    j = json{{"liveness", t.liveness}, {"Conf", t.classify_confidence},
        {"HSV_Conf", t.classifyhsv_confidence}, {"W_Conf", t.wegithed_confidence}};
}
inline void from_json(const json &j, AntiFaceSpoofing::result_t &t) {
    t.liveness = j.at("liveness").get<bool>();
    t.classify_confidence = j.at("Conf").get<float>();
    t.classifyhsv_confidence = j.at("HSV_Conf").get<float>();
    t.wegithed_confidence = j.at("W_Conf").get<float>();
}

}
}

REGISTER_UNIQUE_TYPE_NAME(inference::pipeline::AntiFaceSpoofing::result_t);