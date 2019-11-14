#include "mark_face_sink.hpp"
#include <opencv2/opencv.hpp>

void mark_face(image_t &img, fr_result_t &face) {
    cv::Mat image = img;
    std::string name(face.name.data());
    const qnn::vision::face_info_t &face_info = face.face;
    auto color = cv::Scalar(255, name.empty() ? 0 : 255, 0);
    float x = face_info.bbox.x1;
    float y = face_info.bbox.y1;
    float w = face_info.bbox.x2 - face_info.bbox.x1 + 1;
    float h = face_info.bbox.y2 - face_info.bbox.y1 + 1;

    qnn::vision::face_pts_t facePts = face_info.face_pts;
    for (int j = 0; j < 5; j++) {
        cv::circle(image, cv::Point(facePts.x[j], facePts.y[j]), 1, color, 2);
    }

    cv::rectangle(image, cv::Rect(x, y, w, h), color, 2);
    cv::putText(image, name, cv::Point(x, y), 0, 1, color);
}
