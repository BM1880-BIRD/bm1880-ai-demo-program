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

std::tuple<> FD::process(std::tuple<image_t &, VectorPack &> tuple) {
    std::lock_guard<std::mutex> lock(m);
    std::vector<face_info_t> face_infos;
    fd_->detect(std::get<0>(tuple), face_infos);
    std::get<1>(tuple).add(std::move(face_infos));
    return {};
}

FR::FR(const json &config) {
    fr_.reset(new ::FR(
        config.at("algorithm").get<face_algorithm_t>(),
        config.at("models").get<vector<string>>()
    ));
}

std::tuple<> FR::process(std::tuple<image_t &, VectorPack &> tuple) {
    std::lock_guard<std::mutex> lock(m);
    cv::Mat image = std::get<0>(tuple);
    auto &vp = std::get<1>(tuple);
    std::vector<qnn::vision::FaceAttributeInfo> result(vp.size<face_info_t>());

    for (size_t i = 0; i < result.size(); i++) {
        cv::Mat aligned(112, 112, image.type());
        if (face_align(image, aligned, vp.at<face_info_t>(i), 112, 112) == qnn::RET_SUCCESS) {
            fr_->predict(aligned, result[i]);
        }
    }

    vp.add(std::move(result));
    return {};
}

}
}