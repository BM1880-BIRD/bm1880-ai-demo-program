#include "afs_patch.hpp"
#include "face_api.hpp"
#include <face/anti_facespoofing/afs_patch.hpp>

using namespace std;

namespace inference {
namespace pipeline {

AFSPatch::AFSPatch(const json &config) : AFSBase(config) {
    core.reset(new qnn::vision::AntiFaceSpoofingPatch(config.at("model").get<string>()));
}

result_item_t AFSPatch::getItem(std::tuple<image_t &, VectorPack &> tuple) {
    result_item_t item;
    auto &vp = get<1>(tuple);

    if (!vp.get<face_info_t>().size()) {
        return item;
    }

    auto &bbox = vp.get<face_info_t>()[0].bbox;
    cv::Rect face_rect(bbox.x1, bbox.y1, bbox.x2 - bbox.x1 + 1, bbox.y2 - bbox.y1 + 1);
    cv::Mat image = get<0>(tuple);
    cv::Mat crop_img = image(RectScale(face_rect, image.cols, image.rows, 1.5)).clone();
    bool is_real = false;

    if (!crop_img.empty()) {
        core->Detect(crop_img, is_real);

        item.type = "patch";
        item.liveness = is_real;
    }

    return item;
}

}
}
