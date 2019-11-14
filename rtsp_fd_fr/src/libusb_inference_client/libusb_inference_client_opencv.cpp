#include "libusb_inference_client.hpp"
#include <opencv2/opencv.hpp>
#include <vector>
#include <thread>
#include "data_pipe.hpp"
#include <signal.h>
#include "timer.hpp"
#include "log_common.h"

using namespace std;

DataPipe<cv::Mat> dp(3, data_pipe_mode::blocking);

int main() {
    libusb_inference_init();
    InferenceModel algo = OPENPOSE;
    libusb_inference_load_model(algo);

    signal(SIGINT, [](int sig) {
        dp.close();
        libusb_inference_free();
    });

    cv::VideoCapture vc(0);
    cv::Mat img;

    thread th([]() {
        while (dp.is_opened()) {
            auto img = dp.try_get(chrono::milliseconds(10));
            if (img.has_value()) {
                cv::imshow("1", img.value());
            }
            cv::waitKey(1);
        }
    });

    bool first = true;
    vector<unsigned char> buf;
    vector<object_detect_rect_t> result;
    while (dp.is_opened() && vc.read(img)) {
        cv::Mat resized;
        cv::resize(img, resized, cv::Size(1920, 1080));
        Timer timer("yolo");
        if (!first) {
            timer.tik();
        }
        cv::imencode(".jpg", resized, buf);
        LOGD << "";
        libusb_inference_send(algo, move(buf));
        if (first) {
            LOGD << "";
            first = false;
            continue;
        }
        LOGD << "";
        if (libusb_inference_receive(buf) < 0) {
            LOGD << "";
            break;
        }
        LOGD << "";
        timer.tok();
        cv::Mat show = cv::imdecode(buf, cv::IMREAD_COLOR);
        dp.put(move(show));
    }
    LOGD << "";
    libusb_inference_receive(buf);

    dp.close();
    libusb_inference_free();
    th.join();

    return 0;
}

