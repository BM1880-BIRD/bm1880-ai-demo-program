#pragma once

#include "memory_view.hpp"
#include "encoding.hpp"

class cvmat_image_t;

class bgr_image_t {
    friend class cvmat_image_t;
    friend class Encoding<bgr_image_t>;

    using size_type = int64_t;
    struct info_t {
        size_type height;
        size_type width;
    };
    struct pixel_t {
        unsigned char blue;
        unsigned char green;
        unsigned char red;
    };
    static_assert(sizeof(pixel_t) == 3, "sizeof(pixel_t) must be 3");

    info_t info;
    Memory::SharedView<pixel_t> image_data;

public:
    bgr_image_t() : info({0, 0}) {}
    bgr_image_t(size_type width, size_type height, const Memory::SharedView<pixel_t> &data) : info({height, width}), image_data(data) {
        assert(width * height == image_data.size());
    }
    bgr_image_t(const bgr_image_t &) = default;
    bgr_image_t(bgr_image_t &&) = default;

    bgr_image_t &operator=(const bgr_image_t &) = default;
    bgr_image_t &operator=(bgr_image_t &&) = default;

    bool empty() const {
        return info.width == 0 || info.height == 0;
    }
};

template <>
class Encoding<bgr_image_t> {
public:
    template <typename Inserter>
    static void encode(const bgr_image_t &image, Inserter inserter) {
        std::vector<unsigned char> size_buffer(sizeof(image.info));
        memcpy(size_buffer.data(), &image.info, size_buffer.size());

        inserter = std::move(size_buffer);
        inserter = image.image_data;
    }

    template <typename Iterator>
    static Iterator decode(Iterator begin, Iterator end, bgr_image_t &image) {
        if (begin == end || std::next(begin) == end) {
            return end;
        }

        memcpy(&image.info, begin->data(), sizeof(image.info));
        image.image_data = *std::next(begin);

        return std::next(begin, 2);
    }
};
