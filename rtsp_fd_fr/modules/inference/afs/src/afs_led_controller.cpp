#include "afs_led_controller.hpp"
#include <opencv2/opencv.hpp>
#include "face_api.hpp"

using namespace std;

namespace inference {
namespace pipeline {

LEDController::LEDController(const json &config) : AFSBase(config) {
    mean_threshold = config.at("mean_threshold").get<float>();
    std_dev_threshold = config.at("std_dev_threshold").get<float>();
}

std::tuple<> LEDController::process(std::tuple<image_t &, VectorPack &> tuple) {
    auto &vp = get<1>(tuple);

    if (!vp.get<face_info_t>().size()) {
        return make_tuple();
    }

    auto &bbox = vp.get<face_info_t>()[0].bbox;
    cv::Rect face_rect(bbox.x1, bbox.y1, bbox.x2 - bbox.x1 + 1, bbox.y2 - bbox.y1 + 1);
    cv::Rect scale_bbox = squareRectScale(face_rect, 0.6);
    cv::Mat crop_image = cropImage(get<0>(tuple), scale_bbox);
    cv::Mat tmp_m, tmp_sd;
    cv::meanStdDev(crop_image, tmp_m, tmp_sd);
    double mean = tmp_m.at<double>(0,0);
    double std_dev = tmp_sd.at<double>(0,0);

    if ((mean < mean_threshold) && (std_dev_threshold < std_dev_threshold)) {
        cout << "@@@@@@@@@@@ too dark" << endl;
    } else {
        cout << "@@@@@@@@@@@ ok" << endl;
    }

    return make_tuple();
}

result_item_t LEDController::getItem(std::tuple<image_t &, VectorPack &> tuple) {
    result_item_t item;

    return item;
}

}
}