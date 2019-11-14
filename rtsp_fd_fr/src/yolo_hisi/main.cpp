#include "yolo.hpp"
#include <opencv2/opencv.hpp>
#include <vector>
#include <thread>
#include "data_pipe.hpp"
#include <signal.h>
#include "timer.hpp"

using namespace std;

DataPipe<cv::Mat> dp(3, data_pipe_mode::blocking);

int main() {
    remote_yolo_init();

    signal(SIGINT, [](int sig) {
        dp.close();
        remote_yolo_free();
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
        Timer timer("yolo");
        if (!first) {
            timer.tik();
        }
        cv::imencode(".jpg", img, buf);
        remote_yolo_send(move(buf));
        if (first) {
            first = false;
            continue;
        }
        if (remote_yolo_receive(buf, result) < 0) {
            break;
        }
        timer.tok();
        cv::Mat show = cv::imdecode(buf, cv::IMREAD_COLOR);
        dp.put(move(show));
    }
    remote_yolo_receive(buf, result);

    dp.close();
    remote_yolo_free();
    th.join();

    return 0;
}

