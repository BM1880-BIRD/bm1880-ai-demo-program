#include "afs_depth.hpp"
#include "face_api.hpp"

using namespace std;

namespace inference {
namespace pipeline {

AFSDepth::AFSDepth(const json &config) : AFSBase(config) {
    threshold = config.at("threshold").get<float>();

    core.reset(new qnn::vision::AntiFaceSpoofingDepth(config.at("model").get<string>()));
    core->SetThreshold(threshold);
}

result_item_t AFSDepth::getItem(std::tuple<image_t &, VectorPack &> tuple) {
    result_item_t item;
    auto &vp = get<1>(tuple);

    // if (!vp.get<face_info_t>().size()) {
    if (vp.get<face_info_t>().size() == 0) {
        return item;
    }

    // auto &bbox = vp.get<face_info_t>()[0].bbox;
    auto &bbox = vp.get<face_info_t>()[0].bbox;
    cv::Rect face_rect(bbox.x1, bbox.y1, bbox.x2 - bbox.x1 + 1, bbox.y2 - bbox.y1 + 1);
    cv::Mat image = get<0>(tuple);
    cv::Mat crop_img = image(RectScale(face_rect, image.cols, image.rows, 0.9, 0, 10)).clone();
    bool is_real = false;

    core->Detect(crop_img, is_real);

    item.type = "depth";
    item.liveness = is_real;
    item.threshold = threshold;

    return item;
}

}
}
