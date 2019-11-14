#include "video_capture_source.hpp"
#include <opencv2/opencv.hpp>
#include <string>
#include "logging.hpp"

using namespace std;

VideoCaptureSource::VideoCaptureSource(int device) : VideoCaptureSource(json{{"url", string("/dev/video") + to_string(device)}}) {}
VideoCaptureSource::VideoCaptureSource(const std::string &filename) : VideoCaptureSource(json{{"url", filename}}) {}
VideoCaptureSource::VideoCaptureSource(const json &config) :
DataSource<image_t>("VideoCaptureSource"),
config(config) {
    open();
}

void VideoCaptureSource::open() {
    capture.reset(new cv::VideoCapture(config.at("url").get<std::string>()));

    auto format_json = config.find("format");
    if (format_json != config.end()) {
        std::string format = config.at("format").get<std::string>();
        int n = format.length();
        if (n == 4) {
            char char_array[n+1];
            strcpy(char_array, format.c_str());
            int fourcc = CV_FOURCC(char_array[0], char_array[1], char_array[2], char_array[3]);
            std::cout << "Set FOURCC: " << fourcc << std::endl;
            capture->set(CV_CAP_PROP_FOURCC, fourcc);
        }
    }

    auto fps_json = config.find("fps");
    if (fps_json != config.end()) {
        if (config.at("fps").get<int>() > 0) {
            std::cout << "Set FPS: " << config.at("fps").get<int>() << std::endl;
            capture->set(CV_CAP_PROP_FPS, config.at("fps").get<int>());
        }
    }

    auto width_json = config.find("width");
    auto height_json = config.find("height");
    if (width_json != config.end() && height_json != config.end()) {
        if (config.at("width").get<int>() > 0 && config.at("height").get<int>() > 0) {
            std::cout << "Set W/H: " << config.at("width").get<int>() << config.at("height").get<int>() << std::endl;
            capture->set(CV_CAP_PROP_FRAME_WIDTH, config.at("width").get<int>());
            capture->set(CV_CAP_PROP_FRAME_HEIGHT, config.at("height").get<int>());
        }
    }

    auto get_nonneg_int = [](const json &config, const char *name, int &ref) {
        auto iter = config.find(name);
        if (iter != config.end()) {
            ref = max<int>(0, iter->get<int>());
        }
    };

    get_nonneg_int(config, "retry_grab_count", retry_grab_count);
    get_nonneg_int(config, "retry_open_count", retry_open_count);
    get_nonneg_int(config, "retry_grab_sleep", retry_grab_sleep);
    get_nonneg_int(config, "retry_open_sleep", retry_open_sleep);
}

VideoCaptureSource::~VideoCaptureSource() {
    capture->release();
}

mp::optional<image_t> VideoCaptureSource::get() {
    cv::Mat frame;
    mp::optional<image_t> result;
    bool ok = false;

    for (int i = 0; i <= retry_open_count; i++) {
        if (i > 0) {
            usleep(retry_open_sleep);
            open();
        }
        for (int j = 0; j <= retry_grab_count; j++) {
            if (j > 0) {
                usleep(retry_grab_sleep);
            }
            ok = capture->read(frame);
            if (ok) {
                result.emplace(frame);
                return result;
            } else {
                LOGI << "failed reading frame from VideoCapture";
            }
        }
        close();
    }

    return result;
}

bool VideoCaptureSource::is_opened() {
    return capture->isOpened();
}

void VideoCaptureSource::close() {
    capture->release();
}