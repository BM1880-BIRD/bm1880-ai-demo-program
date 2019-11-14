#include "afs_canny.hpp"
#include "face_api.hpp"

using namespace std;

namespace inference {
namespace pipeline {

AFSCanny::AFSCanny(const json &config) : AFSBase(config) {
    weighting = config.at("weighting").get<float>();
    threshold = config.at("threshold").get<float>();

    core.reset(new qnn::vision::AntiFaceSpoofingCanny(config.at("model").get<string>()));
    core->SetThreshold(threshold);
}

result_item_t AFSCanny::getItem(std::tuple<image_t &, VectorPack &> tuple) {
    result_item_t item;
    auto &vp = get<1>(tuple);

    if (!vp.get<face_info_t>().size()) {
        return item;
    }

    auto &bbox = vp.get<face_info_t>()[0].bbox;
    cv::Rect rect(bbox.x1, bbox.y1, bbox.x2 - bbox.x1 + 1, bbox.y2 - bbox.y1 + 1);
    bool is_real = false;

    core->Detect(get<0>(tuple), rect, is_real);

    item.type = "canny";
    item.liveness = is_real;
    item.confidence = core->GetConfidence();
    item.threshold = threshold;
    item.weighting = weighting;

    return item;
}

}
}
