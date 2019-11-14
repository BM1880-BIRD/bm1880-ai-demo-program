#include "afs_classify.hpp"
#include "face_api.hpp"

using namespace std;

namespace inference {
namespace pipeline {

AFSClassify::AFSClassify(const json &config) : AFSBase(config) {
    weighting = config.at("weighting").get<float>();
    threshold = config.at("threshold").get<float>();
    if (config.find("scaleratio") != config.end()) {
        scale_ratio = config.at("scaleratio").get<float>();
    }

    core.reset(new qnn::vision::AntiFaceSpoofingClassify(config.at("model").get<string>()));
    core->SetThreshold(threshold);
}

result_item_t AFSClassify::getItem(std::tuple<image_t &, VectorPack &> tuple) {
    result_item_t item;
    auto &vp = get<1>(tuple);

    if (!vp.contains<face_info_t>() || !vp.get<face_info_t>().size()) {
        return item;
    }

    auto &bbox = vp.get<face_info_t>()[0].bbox;
    cv::Rect face_rect(bbox.x1, bbox.y1, bbox.x2 - bbox.x1 + 1, bbox.y2 - bbox.y1 + 1);
    bool is_real = false;

    cv::Rect scale_bbox = squareRectScale(face_rect, scale_ratio);
    cv::Mat crop_image = cropImage(get<0>(tuple), scale_bbox);
    cv::Mat resized_image;
    cv::resize(crop_image, resized_image, {224, 224}, 0, 0);

    if (!resized_image.empty()) {
        core->Detect(resized_image, is_real);

        item.type = "classify";
        item.liveness = is_real;
        item.confidence = core->GetConfidence();
        item.threshold = threshold;
        item.weighting = weighting;
    }

    return item;
}

}
}
