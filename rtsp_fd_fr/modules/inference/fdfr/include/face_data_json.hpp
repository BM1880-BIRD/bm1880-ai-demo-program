#pragma once

#include "face_api.hpp"
#include "json.hpp"

void from_json(const json &j, face_algorithm_t &algo);

namespace qnn {
namespace vision {

void to_json(json &j, const face_pts_t &info);
void from_json(const json &j, face_pts_t &info);
void to_json(json &j, const face_detect_rect_t &info);
void from_json(const json &j, face_detect_rect_t &info);
void to_json(json &j, const face_info_t &info);
void from_json(const json &j, face_info_t &info);
void to_json(json &j, const FaceAttributeInfo &info);
void from_json(const json &j, FaceAttributeInfo &info);

}
}