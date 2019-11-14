#include "afs_cvsharp.hpp"
#include "face_api.hpp"

using namespace std;

namespace inference {
namespace pipeline {

AFSCVSharpness::AFSCVSharpness(const json &config) : AFSBase(config) {
    core.reset(new qnn::vision::AntiFaceSpoofingCVSharpness());
    core->SetThreshold(config.at("threshold").get<size_t>());
}

result_item_t AFSCVSharpness::getItem(std::tuple<image_t &, VectorPack &> tuple) {
    result_item_t item;
    auto &vp = get<1>(tuple);

    if (!vp.get<face_info_t>().size()) {
        return item;
    }

    auto &bbox = vp.get<face_info_t>()[0].bbox;
    cv::Rect face_rect(bbox.x1, bbox.y1, bbox.x2 - bbox.x1 + 1, bbox.y2 - bbox.y1 + 1);
    cv::Mat image = get<0>(tuple);
    cv::Mat crop_img = image(RectScale(face_rect, image.cols, image.rows, 0.9, 0, 10)).clone();
    bool is_real = false;

    item.type = "cvsharpness";
    item.liveness = core->Detect(crop_img);
    item.threshold = core->GetThreshold();

    return item;
}

}
}
