#include "fdfr_pipe.hpp"
#include "string_parser.hpp"
#include "vector_pack.hpp"
#include "face_data_json.hpp"

using namespace std;

namespace face {
namespace pipeline {

FD::FD(const json &config) {
    fd_.reset(new ::FD(
        config.at("algorithm").get<face_algorithm_t>(),
        config.at("models").get<vector<string>>()
    ));
}

FD::FD(face_algorithm_t algo, const std::vector<std::string> &models) {
    fd_.reset(new ::FD(algo, models));
}

tuple<vector<face_info_t>> FD::process(tuple<image_t &> tuple) {
    lock_guard<mutex> lock(m);
    vector<face_info_t> face_infos;
    fd_->detect(get<0>(tuple), face_infos);
    return make_tuple(move(face_infos));
}

FR::FR(const json &config) {
    fr_.reset(new ::FR(
        config.at("algorithm").get<face_algorithm_t>(),
        config.at("models").get<vector<string>>()
    ));
}

FR::FR(face_algorithm_t algo, const std::vector<std::string> &models) {
    fr_.reset(new ::FR(algo, models));
}

tuple<vector<qnn::vision::FaceAttributeInfo>> FR::process(tuple<image_t &, vector<face_info_t> &> tuple) {
    lock_guard<mutex> lock(m);
    cv::Mat image = get<0>(tuple);
    auto &face_info = get<1>(tuple);
    vector<qnn::vision::FaceAttributeInfo> result(face_info.size());

    for (size_t i = 0; i < result.size(); i++) {
        cv::Mat aligned(112, 112, image.type());
        if (face_align(image, aligned, face_info[i], 112, 112) == qnn::RET_SUCCESS) {
            fr_->predict(aligned, result[i]);
        }
    }

    return make_tuple(move(result));
}

}
}