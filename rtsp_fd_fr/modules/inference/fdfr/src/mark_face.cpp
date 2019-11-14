#include "mark_face.hpp"
#include "qnn.hpp"
#include <opencv2/opencv.hpp>
#include <functional>
#include "mp/optional.hpp"

using namespace std;

void mark_face(image_t &image, const face_info_t &face, const string &name, double font_scale) {
    cv::Mat img = image;
    auto color = cv::Scalar(255, name.empty() ? 0 : 255, 0);
    float x = face.bbox.x1;
    float y = face.bbox.y1;
    float w = face.bbox.x2 - face.bbox.x1 + 1;
    float h = face.bbox.y2 - face.bbox.y1 + 1;

    qnn::vision::face_pts_t facePts = face.face_pts;
    for (int j = 0; j < 5; j++) {
        cv::circle(img, cv::Point(facePts.x[j], facePts.y[j]), 1, color, 2);
    }

    cv::rectangle(img, cv::Rect(x, y, w, h), color, 2);
    cv::putText(img, name, cv::Point(x, y - 3), 0, font_scale, color);
}

tuple<> FaceResultVisualizer::process(tuple<image_t &, const VectorPack &> args) {
    using namespace qnn::vision;

    cv::Mat image = get<0>(args);
    auto &result = get<1>(args);

    if (!result.contains<face_info_t>()) {
        return {};
    }

    auto color = cv::Scalar(255, 0, 0);
    auto &face_infos = result.get<face_info_t>();
    mp::optional<reference_wrapper<const vector<FaceAttributeInfo>>> face_attrs;
    if (result.contains<FaceAttributeInfo>()) {
        face_attrs.emplace(cref(result.get<FaceAttributeInfo>()));
    }

    for (int i = 0; i < face_infos.size(); i++) {
        auto &face_info = face_infos[i];
        float x = face_info.bbox.x1;
        float y = face_info.bbox.y1;
        float w = face_info.bbox.x2 - face_info.bbox.x1 + 1;
        float h = face_info.bbox.y2 - face_info.bbox.y1 + 1;
        cv::rectangle(image, cv::Rect(x, y, w, h), color, 2);
        for (int j = 0; j < 5; j++) {
            cv::circle(image, cv::Point(face_info.face_pts.x[j], face_info.face_pts.y[j]), 1, color, 2);
        }

        if (face_attrs.has_value() && i < face_attrs.value().get().size()) {
            auto &attr = face_attrs.value().get().at(i);
            cv::putText(image, GetEmotionString(FaceEmotion(attr.emotion)).c_str(), cv::Point(x, y), 2, 1, color);
            y += 30;
            cv::putText(image, GetGenderString(FaceGender(attr.gender)).c_str(), cv::Point(x, y), 2, 1, color);
            y += 30;
            cv::putText(image, GetRaceString(FaceRace(attr.race)).c_str(), cv::Point(x, y), 2, 1, color);
            y += 30;
            cv::putText(image, to_string(int(attr.age)).c_str(), cv::Point(x, y), 2, 1, color);
        }
    }

    return {};
}