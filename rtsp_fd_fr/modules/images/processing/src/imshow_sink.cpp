#include "imshow_sink.hpp"
#include <opencv2/opencv.hpp>
#include "logging.hpp"

bool ImshowSink::put(image_t &&frame) {
    try {
        std::lock_guard<std::mutex> locker(lock);
        if (closed) {
            return false;
        }
        cv::imshow(window_name_, (cv::Mat)frame);
        cv::waitKey(1);
    } catch (cv::Exception e) {
        LOGD << "cv::Exception in ImshowSink: " << e.what();
    }
    return true;
}

void ImshowSink::close() {
    std::lock_guard<std::mutex> locker(lock);
    closed = true;
    cv::destroyAllWindows();
}

bool ImshowSink::is_opened() {
    std::lock_guard<std::mutex> locker(lock);
    return !closed;
}
