#include "face_attr_pipe.hpp"

using namespace std;

namespace face {
namespace pipeline {

static inline qnn::vision::StabilizeMethod StringToStabilizeMethod(const string &s) {
    if (s == "average") {
        return qnn::vision::StabilizeMethod::STABILIZE_METHOD_AVERAGE;
    } else if (s == "vote") {
        return qnn::vision::StabilizeMethod::STABILIZE_METHOD_VOTE;
    } else if (s == "weighted_sum") {
        return qnn::vision::StabilizeMethod::STABILIZE_METHOD_WEIGHTED_SUM;
    } else {
        throw runtime_error("invalid stabilize method");
    }
}

FaceAttrStabilizer::FaceAttrStabilizer(const json &config) :
stablizer(
    StringToStabilizeMethod(config.at("gender_method").get<string>()),
    config.at("gender_queue_size").get<size_t>(),
    config.at("gender_reset_time").get<double>(),
    StringToStabilizeMethod(config.at("race_method").get<string>()),
    config.at("race_queue_size").get<size_t>(),
    config.at("race_reset_time").get<double>(),
    StringToStabilizeMethod(config.at("age_method").get<string>()),
    config.at("age_queue_size").get<size_t>(),
    config.at("age_reset_time").get<double>(),
    StringToStabilizeMethod(config.at("emotion_method").get<string>()),
    config.at("emotion_queue_size").get<size_t>(),
    config.at("emotion_reset_time").get<double>()),
repo(make_shared<Repository>(config.at("repository").get<string>())),
similarity_threshold(config.at("similarity_threshold").get<double>()),
strangers_lru(config.at("stranger_count").get<size_t>()),
stranger_similarity_threshold(config.at("stranger_similarity_threshold").get<double>())
{}

tuple<> FaceAttrStabilizer::process(tuple<image_t &, VectorPack &> tuple) {
    auto &vp = get<1>(tuple);

    for (size_t i = 0; i < vp.size<face_features_t>(); i++) {
        auto matched = repo->match(vp.at<face_features_t>(i).face_feature.features, similarity_threshold);
        string name;
        face_features_t stablized_feature;

        if (matched.matched) {
            stablizer.StabilizeFaceAttribute({true, matched.id}, vp.at<face_features_t>(i), &stablized_feature);
            vp.at<face_features_t>(i) = stablized_feature;
        } else {
            auto match = stranger_repo.match(vp.at<face_features_t>(i).face_feature.features, stranger_similarity_threshold);
            if (!match.matched) {
                match.id = stranger_repo.add("", vp.at<face_features_t>(i).face_feature.features);
                auto removed = strangers_lru.put(match.id);
                if (removed.has_value()) {
                    stranger_repo.remove(removed.value());
                    stablizer.remove({false, removed.value()});
                }
            }
            face_features_t stablized_feature;
            stablizer.StabilizeFaceAttribute({false, match.id}, vp.at<face_features_t>(i), &stablized_feature);
            vp.at<face_features_t>(i) = stablized_feature;
        }
    }

    return {};
}

}
}