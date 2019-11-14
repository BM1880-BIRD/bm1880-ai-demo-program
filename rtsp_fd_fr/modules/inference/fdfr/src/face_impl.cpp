#include "face_impl.hpp"

using namespace std;
using namespace qnn::vision;

memory::Iterable<fr_result_t> FaceImpl::faceid(const image_t &image) {
    vector<face_info_t> face_infos;
    vector<fr_result_t> face_result;
    vector<face_features_t> features_list;
    fdfr_.detect_extract(image, face_infos, features_list);
    face_result.resize(face_infos.size());

    for (size_t i = 0; i < face_infos.size(); i++) {
        face_result[i] = gen_fr_result(face_infos[i], features_list[i]);
        memset(face_result[i].name.data(), 0, face_result[i].name.size());
        auto match_result = repo_.match(features_list[i].face_feature.features, similarity_threshold);
        strncpy(face_result[i].name.data(), repo_.get_match_name(match_result).data(), face_result[i].name.size());
    }

    return memory::Iterable<fr_result_t>(memory::Bytes(move(face_result)));
}


memory::Iterable<fr_result_t> FaceImpl::faceinfo(const image_t &image ,double* score){
    vector<face_info_t> face_infos;
    vector<fr_result_t> face_result;
    vector<face_features_t> features_list;
    fdfr_.detect_extract(image, face_infos, features_list);
    face_result.resize(face_infos.size());

    for (size_t i = 0; i < face_infos.size(); i++) {
        face_result[i] = gen_fr_result(face_infos[i], features_list[i]);
        memset(face_result[i].name.data(), 0, face_result[i].name.size());
        auto match_result = repo_.match(features_list[i].face_feature.features, 0.1);
        strncpy(face_result[i].name.data(), repo_.get_match_name(match_result).data(), face_result[i].name.size());
        *score = match_result.score;
    }

    return memory::Iterable<fr_result_t>(memory::Bytes(move(face_result)));
}


bool FaceImpl::register_face(const std::string &name, image_t &&image) {
    vector<face_info_t> face_infos;
    vector<fr_result_t> face_result;
    vector<face_features_t> features_list;

    printf("start to register_face\n");
    fdfr_.detect_extract(image, face_infos, features_list);
    printf("register_face done\n");
    if (features_list.size() == 1) {
        printf("save features\n");
        repo_.add(name, features_list[0].face_feature.features);
        return true;
    } else {
        return false;
    }
}

memory::Iterable<fd_result_t> FaceImpl::detect_face(const image_t &image) {
    vector<fd_result_t> face_infos;
    fdfr_.detect(image, face_infos);
    return memory::Iterable<fd_result_t>(memory::Bytes(move(face_infos)));
}
