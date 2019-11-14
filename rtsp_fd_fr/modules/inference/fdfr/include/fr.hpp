#pragma once

#include <array>
#include <string>
#include <vector>
#include "qnn.hpp"
#include "image.hpp"

class FR {
public:
    FR(face_algorithm_t algo, const std::vector<std::string> &models) {
        assert(algo == FR_BMFACE);

        auto face_attribute = std::unique_ptr<qnn::vision::FaceAttribute>(new qnn::vision::FaceAttribute(models[0]));
        face_attribute->EnableDequantize(false);
        recognizer = std::move(face_attribute);
    }

    virtual ~FR() {}

    void predict(const cv::Mat &image, qnn::vision::FaceAttributeInfo &results) {
        recognizer->Detect(image, results);
    }

private:
    std::unique_ptr<qnn::vision::FaceRecognizer> recognizer;
};
