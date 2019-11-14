#include "face_data_json.hpp"
#include <string>

void from_json(const json &j, face_algorithm_t &algo) {
    auto s = j.get<std::string>();
    if (s == "tinyssh") {
        algo = FD_TINYSSH;
    } else if (s == "ssh") {
        algo = FD_SSH;
    } else if (s == "mtcnn") {
        algo = FD_MTCNN;
    } else if (s == "mtcnn_native") {
        algo = FD_MTCNN_NATIVE;
    } else if (s == "bmface") {
        algo = FR_BMFACE;
    } else {
        algo = INVALID_ALOG;
    }
}

namespace qnn {
namespace vision {

void to_json(json &j, const face_pts_t &info) {
    j = json{{"x", info.x}, {"y", info.y}};
}
void from_json(const json &j, face_pts_t &info) {
    j.at("x").get_to(info.x);
    j.at("y").get_to(info.y);
}

void to_json(json &j, const face_detect_rect_t &info) {
    j = json{
        {"x1", info.x1},
        {"y1", info.y1},
        {"x2", info.x2},
        {"y2", info.y2},
        {"id", info.id},
        {"score", info.score},
    };
}
void from_json(const json &j, face_detect_rect_t &info) {
    j.at("x1").get_to(info.x1);
    j.at("y1").get_to(info.y1);
    j.at("x2").get_to(info.x2);
    j.at("y2").get_to(info.y2);
    j.at("id").get_to(info.id);
    j.at("score").get_to(info.score);
}

void to_json(json &j, const face_info_t &info) {
    j = json{{"bbox", info.bbox}, {"points", info.face_pts}};
}
void from_json(const json &j, face_info_t &info) {
    j.at("bbox").get_to(info.bbox);
    j.at("points").get_to(info.face_pts);
}

void to_json(json &j, const FaceAttributeInfo &info) {
    j = json{
        {"face_feature", info.face_feature.features},
        {"emotion_prob", info.emotion_prob.features},
        {"gender_prob", info.gender_prob.features},
        {"race_prob", info.race_prob.features},
        {"age_prob", info.age_prob.features},
        {"emotion", info.emotion},
        {"gender", info.gender},
        {"race", info.race},
        {"age", info.age},
    };
}
void from_json(const json &j, FaceAttributeInfo &info) {
    j.at("face_feature").get_to(info.face_feature.features);
    j.at("emotion_prob").get_to(info.emotion_prob.features);
    j.at("gender_prob").get_to(info.gender_prob.features);
    j.at("race_prob").get_to(info.race_prob.features);
    j.at("age_prob").get_to(info.age_prob.features);
    j.at("emotion").get_to(info.emotion);
    j.at("gender").get_to(info.gender);
    j.at("race").get_to(info.race);
    j.at("age").get_to(info.age);
}

}
}