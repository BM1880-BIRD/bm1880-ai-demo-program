#pragma once

#include <cstdint>
#include "memory_view.hpp"
#include "encoding.hpp"
#include "http_encode.hpp"

#ifdef OPENCV_DISABLED
    #include "bgr_image.hpp"
    using image_t = bgr_image_t;
#else
    #include "cvmat_image.hpp"
    using image_t = cvmat_image_t;
#endif
