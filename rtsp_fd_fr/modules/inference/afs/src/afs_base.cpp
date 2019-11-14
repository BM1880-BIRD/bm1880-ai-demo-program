#include "afs_base.hpp"
#include <string>
#include <opencv2/opencv.hpp>
#include "face_api.hpp"

using namespace std;

namespace inference {
namespace pipeline {

AFSBase::AFSBase(const json &config) {
}

std::tuple<> AFSBase::process(std::tuple<image_t &, VectorPack &> tuple) {
    auto &vp = get<1>(tuple);
    result_items_t items;

    if (vp.contains<result_items_t>()) {
        items = vp.get<result_items_t>()[0];
    }

    if (vp.contains<face_info_t>()) {
        items.data.emplace_back(getItem(tuple));
    }
    vp.add(vector<result_items_t>{items});

    return make_tuple();
}

cv::Rect AFSBase::squareRectScale(const cv::Rect &rect, const float scale_val) {
    cv::Rect bbox = rect;
    cv::Rect square_bbox;

    if (bbox.width != bbox.height) {
        int max_length = std::max(bbox.width, bbox.height);
        square_bbox = cv::Rect(bbox.x + bbox.width/2 - max_length/2,
                               bbox.y + bbox.height/2 - max_length/2,
                               max_length, max_length);
    } else {
        square_bbox = bbox;
    }

    float new_length = square_bbox.width * scale_val;
    return cv::Rect(square_bbox.x + square_bbox.width/2 - new_length/2,
                    square_bbox.y + square_bbox.height/2 - new_length/2,
                    new_length, new_length);
}

cv::Mat AFSBase::cropImage(const cv::Mat &image, const cv::Rect &face_rect) {
    int offset = 0;
    cv::Rect bbox = face_rect;

    // Prevent image distortion
    if ((bbox.width > image.cols) || (bbox.height > image.rows)) {
        int new_left = std::max(0, bbox.x);
        int new_top = std::max(0, bbox.y);
        int new_right = std::min(image.cols, bbox.x + bbox.width);
        int new_bottom = std::min(image.rows, bbox.y + bbox.height);

        int new_width = new_right - new_left;
        int new_height = new_bottom - new_top;

        if (new_width > new_height) {
            int center_x = (new_left + new_right) / 2;
            new_left = center_x - new_height / 2;
            new_right = center_x + new_height / 2;
        } else if (new_height > new_width) {
            int center_y = (new_bottom + new_top) / 2;
            new_top = center_y - new_width / 2;
            new_bottom = center_y + new_width / 2;
        }
        bbox = cv::Rect(new_left, new_top, new_right - new_left, new_bottom - new_top);
    } else {
        if ((bbox.x + bbox.width) >= image.cols) {
            offset = bbox.x + bbox.width - image.cols - 1;
            bbox.x = std::max(0, bbox.x - offset);
            bbox.width = (image.cols - 1) - bbox.x;
        }
        if (bbox.x < 0) {
            bbox.x = 0;
            bbox.width = std::min(image.cols, bbox.width);
        }
        if ((bbox.y + bbox.height) >= image.rows) {
            offset = (bbox.y + bbox.height) - (image.rows - 1);
            bbox.y = std::max(0, bbox.y - offset);
            bbox.height = (image.rows - 1) - bbox.y;
        }
        if (bbox.y < 0) {
            bbox.y = 0;
            bbox.height = std::min(image.rows, bbox.height);
        }
    }

    return image(bbox).clone();
}

cv::Rect AFSBase::RectScale(const cv::Rect &rect, const int cols, const int rows,
                            const float scale_val, const int offset_x, const int offset_y) {
    cv::Rect new_rect = rect;
    int cx = new_rect.x + new_rect.width / 2 + offset_x;
    int cy = new_rect.y + new_rect.height / 2 + offset_y;
    new_rect.width *= scale_val;
    new_rect.height *= scale_val;
    new_rect.x = cx - new_rect.width / 2;
    new_rect.y = cy - new_rect.height / 2;
    if (new_rect.x < 0) new_rect.x = 0;
    if (new_rect.y < 0) new_rect.y = 0;
    if (new_rect.x + new_rect.width > cols) new_rect.width = cols - new_rect.x - 1;
    if (new_rect.y + new_rect.height > rows) new_rect.height = rows - new_rect.y - 1;
    return new_rect;
}

}
}