#include "qnn.hpp"
#include "mark_face_attr_sink.hpp"
#include <opencv2/opencv.hpp>

using namespace qnn::vision;

void mark_attr(image_t &img, fr_result_t &face) {
    if (strncmp(face.name.data(), "Unknown", 7) == 0) {
        return;
    }
    cv::Mat image = img;
    const qnn::vision::face_info_t &face_info = face.face;
    auto color = cv::Scalar(255, 0, 0);
    float x = face_info.bbox.x1;
    float y = face_info.bbox.y1;
    float w = face_info.bbox.x2 - face_info.bbox.x1 + 1;
    float h = face_info.bbox.y2 - face_info.bbox.y1 + 1;

    cv::rectangle(image, cv::Rect(x, y, w, h), color, 2);
    /*
    std::string name(face.name.data());
    cv::putText(image, name, cv::Point(x, y), 2, 1, color);
    y += 30;
    cv::putText(image, GetEmotionString(FaceEmotion(face.emotion)).c_str(), cv::Point(x, y), 2, 1, color);
    y += 30;
    */
    cv::putText(image, GetGenderString(FaceGender(face.gender)).c_str(), cv::Point(x, y), 2, 1, color);
    y += 30;
    cv::putText(image, GetRaceString(FaceRace(face.race)).c_str(), cv::Point(x, y), 2, 1, color);
    y += 30;
    cv::putText(image, std::to_string(int(face.age)).c_str(), cv::Point(x, y), 2, 1, color);
}
