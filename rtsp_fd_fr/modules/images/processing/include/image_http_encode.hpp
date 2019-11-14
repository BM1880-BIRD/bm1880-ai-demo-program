#pragma once

#include "image.hpp"
#include "http_encode.hpp"

template <>
struct HttpEncode<cvmat_bgr_image_t> {
    static const char *mimetype() {
        return "image/jpeg";
    }
    static memory::Bytes encode(cvmat_bgr_image_t &&img) {
        std::vector<unsigned char> buf;
        cv::imencode(".jpg", (cv::Mat)img, buf);
        return memory::Bytes(move(buf));
    }
};
