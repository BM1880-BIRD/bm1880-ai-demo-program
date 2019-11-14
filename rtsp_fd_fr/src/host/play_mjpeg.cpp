#include <memory>
#include <opencv2/opencv.hpp>
#include "bt.hpp"
#include "image.hpp"
#include "fdfr_version.h"

using namespace std;
using namespace cv;

int main(int argc, char* argv[]) {
    google::InitGoogleLogging(argv[0]);
    FDFRVersion::dump_version();

    signal(SIGSEGV, [] (int sig) -> void { print_backtrace(); });
    signal(SIGABRT, [] (int sig) -> void { print_backtrace(); });

    if (argc != 2) {
        printf("usage: %s stream\n", argv[0]);
        return 0;
    }

    auto video_src = make_shared<VideoCapture>(argv[1]);
    cv::Mat frame;
    cv::namedWindow( "Output", cv::WINDOW_AUTOSIZE );

    while (video_src->read(frame)) {
        cv::imshow("Output", frame);
        int k = cv::waitKey(1);

        // esc
        if (k == 27) {
            break;
        }
    }

    fprintf(stderr, "terminating\n");
    cv::destroyWindow("Output");
    return 0;
}
