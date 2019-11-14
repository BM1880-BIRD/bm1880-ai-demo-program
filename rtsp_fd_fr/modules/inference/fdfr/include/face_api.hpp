#pragma once

#include <future>
#include <vector>
#include "json.hpp"
#include "image.hpp"
#include "memory/bytes.hpp"
#include "face/face_types.hpp"
#include "vector_pack.hpp"

#define FACE_FEATURES_DIM 512

typedef std::array<char, 128> fr_name_t;

typedef enum
{
  FD_TINYSSH = 0,
  FD_MTCNN,
  FD_MTCNN_NATIVE,
  FD_SSH,
  FR_BMFACE = 50,
  INVALID_ALOG
} face_algorithm_t;

typedef qnn::vision::face_info_t face_info_t;
typedef qnn::vision::face_pts_t face_pts_t;
typedef face_info_t fd_result_t;
typedef qnn::vision::FaceAttributeInfo face_features_t;

REGISTER_UNIQUE_TYPE_NAME(qnn::vision::face_info_t)
REGISTER_UNIQUE_TYPE_NAME(qnn::vision::face_pts_t)
REGISTER_UNIQUE_TYPE_NAME(qnn::vision::FaceAttributeInfo)
REGISTER_UNIQUE_TYPE_NAME(fr_name_t)

struct fr_result_t {
    std::array<char, 128> name;
    qnn::vision::face_info_t face;
	std::array<float, FACE_FEATURES_DIM> features;
    float age;
    int8_t emotion;
    int8_t gender;
    int8_t race;
};

class TtyIO;

class FaceAPI {
public:
    virtual memory::Iterable<fr_result_t> faceid(const image_t &image) = 0;
    virtual bool register_face(const std::string &name, image_t &&image) = 0;
    virtual memory::Iterable<fd_result_t> detect_face(const image_t &image) = 0;
    //virtual std::pair<std::string, double> faceinfo(const image_t &image) = 0;
};

class AsyncInterface {
public:
    virtual std::future<memory::Iterable<fr_result_t>> faceid(const image_t &image) = 0;
    virtual std::future<bool> register_face(const std::string &name, image_t &&image) = 0;
    virtual std::future<memory::Iterable<fd_result_t>> detect_face(const image_t &image) = 0;
    //virtual std::pair<std::string, double> faceinfo(const image_t &image) = 0;
};

inline fr_result_t&& gen_fr_result(face_info_t &info, face_features_t &features) {
    fr_result_t result{"", info, {}, features.age, features.emotion, features.gender, features.race};
    memcpy(result.features.data(), features.face_feature.features.data(), sizeof(result.features));
    return std::move(result);
}

#include "face_data_json.hpp"
#include "string_parser.hpp"

template <>
inline face_algorithm_t parse_string<face_algorithm_t>(const std::string &s) {
    if (s == "tinyssh") {
        return FD_TINYSSH;
    } else if (s == "ssh") {
        return FD_SSH;
    } else if (s == "mtcnn") {
        return FD_MTCNN;
    } else if (s == "mtcnn_native") {
        return FD_MTCNN_NATIVE;
    } else if (s == "bmface") {
        return FR_BMFACE;
    } else {
        return INVALID_ALOG;
    }
}
