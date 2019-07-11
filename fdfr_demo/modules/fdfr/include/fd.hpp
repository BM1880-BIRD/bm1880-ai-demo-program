#pragma once

#include <string>
#include <vector>
#include "qnn.hpp"
#include "image.hpp"
#include "face_api.hpp"

class FD {
public:
    FD(face_algorithm_t algo, const std::vector<std::string> &models) {
        if (algo == FD_TINYSSH) {
            detector = std::unique_ptr<qnn::vision::FdSSH>(new qnn::vision::FdSSH(models, qnn::vision::TINY, 320, 320, qnn::vision::UP_LEFT));
        } else if (algo == FD_SSH) {
            detector = std::unique_ptr<qnn::vision::FdSSH>(new qnn::vision::FdSSH(models, qnn::vision::SSH_TYPE::ORIGIN, 320, 320, qnn::vision::UP_LEFT));
        } else if (algo == FD_MTCNN) {
            detector = std::unique_ptr<qnn::vision::Mtcnn>(new qnn::vision::Mtcnn(models, qnn::TENSORSEQ::NCWH));
        } else if (algo == FD_MTCNN_NATIVE) {
            detector = std::unique_ptr<qnn::vision::Mtcnn>(new qnn::vision::Mtcnn(models));
        } else {
            detector = std::unique_ptr<qnn::vision::FdSSH>(new qnn::vision::FdSSH(models, qnn::vision::TINY, 320, 320, qnn::vision::UP_LEFT));
        }
    }

    ~FD() {}

    inline void detect(const image_t &image, std::vector<qnn::vision::face_info_t> &results) {
        detector->Detect(image, results);
    }

private:
    std::unique_ptr<qnn::vision::FaceDetector> detector;
};
