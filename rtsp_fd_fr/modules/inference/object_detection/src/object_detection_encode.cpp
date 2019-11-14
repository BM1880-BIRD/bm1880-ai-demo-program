#include "object_detection_encode.hpp"
#include <string>

namespace qnn {
namespace vision {

void to_json(json &j, const object_detect_rect_t &info) {
    j = json{
        {"x1", info.x1},
        {"y1", info.y1},
        {"x2", info.x2},
        {"y2", info.y2},
        {"classes", info.classes},
        {"prob", info.prob},
        {"label", info.label},
    };
}

void from_json(const json &j, object_detect_rect_t &info) {
    j.at("x1").get_to(info.x1);
    j.at("y1").get_to(info.y1);
    j.at("x2").get_to(info.x2);
    j.at("y2").get_to(info.y2);
    j.at("classes").get_to(info.classes);
    j.at("prob").get_to(info.prob);
    j.at("label").get_to(info.label);
}

}
}