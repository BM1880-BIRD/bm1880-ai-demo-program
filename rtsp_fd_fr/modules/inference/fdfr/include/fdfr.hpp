#pragma once

#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <mutex>
#include "net_core.hpp"
#include "fd.hpp"
#include "fr.hpp"
#include "timer.hpp"
#include "image.hpp"

class FDFR {
public:
    FDFR(face_algorithm_t fd_algo, const std::vector<std::string> &fd_models, face_algorithm_t fr_algo, const std::vector<std::string> &fr_models) : fd_time("fd"), fr_time("fr"), align_time("align") {
        fd = new FD(fd_algo, fd_models);
        fr = new FR(fr_algo, fr_models);
    }
    FDFR(const FDFR &) = delete;

    virtual ~FDFR() {
        delete fd;
        delete fr;
    }

    virtual void detect(const cv::Mat &image, std::vector<qnn::vision::face_info_t> &face_infos) {
        std::lock_guard<std::mutex> lock(fdfr_lock);
        std::vector<qnn::vision::face_info_t> results;
        fd_time.tik();
        fd->detect(image, results);
        printf("detect results:%ld\n", results.size());
        face_infos.swap(results);
        fd_time.tok();
    }

    virtual void extract(const cv::Mat &image, const std::vector<qnn::vision::face_info_t> &face_infos, std::vector<face_features_t> &features) {
        std::lock_guard<std::mutex> lock(fdfr_lock);
        std::vector<cv::Mat> aligned_images;

        features.clear();

        for (const qnn::vision::face_info_t &face : face_infos) {
            cv::Mat aligned(112, 112, image.type());
            align_time.tik();
            if (face_align(image, aligned, face, 112, 112) == qnn::RET_SUCCESS) {
                aligned_images.emplace_back(aligned);
            }
            align_time.tok();
        }

        for (auto &aligned: aligned_images) {
            fr_time.tik();
            qnn::vision::FaceAttributeInfo tmp;
            fr->predict(aligned, tmp);
            features.emplace_back(face_features_t(std::move(tmp)));
            fr_time.tok();
        }
    }

    virtual void detect_extract(const cv::Mat &image, std::vector<qnn::vision::face_info_t> &face_infos, std::vector<face_features_t> &features) {
        detect(image, face_infos);
        extract(image, face_infos, features);
    }

    Timer fd_time;
    Timer fr_time;
    Timer align_time;

protected:
    std::mutex fdfr_lock;
    FD *fd;
    FR *fr;
};
