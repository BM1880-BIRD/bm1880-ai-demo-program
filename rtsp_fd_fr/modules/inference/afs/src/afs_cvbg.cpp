#include "afs_cvbg.hpp"
#include "face_api.hpp"

using namespace std;

namespace inference {
namespace pipeline {

AFSCVBG::AFSCVBG(const json &config) : AFSBase(config) {
    core.reset(new qnn::vision::AntiFaceSpoofingCVBG());
}

result_item_t AFSCVBG::getItem(std::tuple<image_t &, VectorPack &> tuple) {
    result_item_t item;
    auto &vp = get<1>(tuple);
    auto &bbox = vp.get<face_info_t>()[0].bbox;
    cv::Rect face_rect(bbox.x1, bbox.y1, bbox.x2 - bbox.x1 + 1, bbox.y2 - bbox.y1 + 1);
    bool is_real = core->BGDetect(get<0>(tuple), {face_rect});

    item.type = "cvbg";
    item.liveness = is_real;

    return item;
}

}
}
