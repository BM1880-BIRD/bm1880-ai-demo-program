#include "stablized_face_impl.hpp"

using namespace std;
using namespace qnn::vision;

Memory::SharedView<fr_result_t> StablizedFaceImpl::faceid(const image_t &image) {
    vector<face_info_t> face_infos;
    Memory::View<fr_result_t> face_result;
    vector<face_features_t> features_list;
    fdfr_.detect_extract(image, face_infos, features_list);
    face_result.resize(face_infos.size());

    for (size_t i = 0; i < face_infos.size(); i++) {
        auto matched = repo_.match(features_list[i].face_feature.features, similarity_threshold);
        std::string name;

        if (matched.matched) {
            face_features_t stablized_feature;
            stablizer.StabilizeFaceAttribute({true, matched.id}, features_list[i], &stablized_feature);
            features_list[i] = stablized_feature;
            name = repo_.get_match_name(matched);
        } else {
            auto match = stranger_repo.match(features_list[i].face_feature.features, similarity_threshold);
            if (!match.matched) {
                match.id = stranger_repo.add("", features_list[i].face_feature.features);
                auto removed = strangers_lru.put(match.id);
                if (removed.has_value()) {
                    stranger_repo.remove(removed.value());
                    stablizer.remove({false, removed.value()});
                }
            }
            face_features_t stablized_feature;
            stablizer.StabilizeFaceAttribute({false, match.id}, features_list[i], &stablized_feature);
            features_list[i] = stablized_feature;
            name = "stranger: " + to_string(match.id);
        }
        face_result[i] = gen_fr_result(face_infos[i], features_list[i]);
        memset(face_result[i].name.data(), 0, face_result[i].name.size());
        strncpy(face_result[i].name.data(), name.data(), face_result[i].name.size());
    }

    return face_result.to_shared();
}
