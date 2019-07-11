#pragma once

#include <opencv2/opencv.hpp>
#include "bgr_image.hpp"

class cvmat_image_t {
private:
    cv::Mat mat_;

public:
    cvmat_image_t() {}
    cvmat_image_t(const cvmat_image_t &other) = default;
    cvmat_image_t(cvmat_image_t &&other) = default;
    cvmat_image_t(cv::Mat &&mat) {
        *this = std::move(mat);
    }
    cvmat_image_t(const cv::Mat &mat) : cvmat_image_t(cv::Mat(mat)) {}
    cvmat_image_t(const bgr_image_t &img) {
        *this = cv::Mat(img.info.height, img.info.width, CV_8UC3, (void*)img.image_data.data());
    }

    cvmat_image_t &operator=(const cvmat_image_t &other) = default;
    cvmat_image_t &operator=(cvmat_image_t &&other) = default;
    cvmat_image_t &operator=(cv::Mat &&mat) {
        if (mat.type() != CV_8UC3) {
            mat.convertTo(mat, CV_8UC3);
        }

        if (mat.u == nullptr || mat.u->refcount == 0) {
            mat.copyTo(mat_);
        } else {
            mat_ = mat;
        }

        return *this;
    }
    cvmat_image_t &operator=(const cv::Mat &mat) {
        return (*this = cv::Mat(mat));
    }

    operator const cv::Mat &() const {
        return mat_;
    }
    operator cv::Mat &() {
        return mat_;
    }
    operator bgr_image_t() const {
        cv::Mat tmp;
        if (mat_.isContinuous()) {
            tmp = mat_;
        } else {
            mat_.copyTo(tmp);
        }

        return bgr_image_t(mat_.size[1], mat_.size[0], Memory::SharedView<bgr_image_t::pixel_t>(std::move(tmp), tmp.ptr(0), 0, tmp.elemSize() * tmp.total()));
    }

    bool empty() const {
        return mat_.empty();
    }
};

template <>
class Encoding<cvmat_image_t> {
public:
    template <typename Inserter>
    static void encode(const cvmat_image_t &image, Inserter inserter) {
        Encoding<bgr_image_t>::encode(bgr_image_t(image), inserter);
    }

    template <typename Iterator>
    static Iterator decode(Iterator begin, Iterator end, cvmat_image_t &image) {
        if (begin == end || std::next(begin) == end) {
            return end;
        }

        bgr_image_t tmp_image;
        auto retv = Encoding<bgr_image_t>::decode(begin, end, tmp_image);
        image = tmp_image;

        return retv;
    }
};

template <>
struct HttpEncode<cvmat_image_t> {
    static const char *mimetype() {
        return "image/jpeg";
    }
    static Memory::SharedView<unsigned char> encode(cvmat_image_t &&img) {
        std::vector<unsigned char> buf;
        cv::imencode(".jpg", (cv::Mat)img, buf);
        return Memory::SharedView<unsigned char>(move(buf));
    }
};
