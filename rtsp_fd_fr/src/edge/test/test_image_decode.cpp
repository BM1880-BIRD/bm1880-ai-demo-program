

#include <opencv2/opencv.hpp>
#include <string>
#include "bt.hpp"

using namespace cv;
using namespace std;

int main(int argc, char* argv[]) {
    signal(SIGSEGV, [] (int sig) -> void { print_backtrace(); });
    signal(SIGABRT, [] (int sig) -> void { print_backtrace(); });

    if (argc != 3) {
        printf("usage: %s input_image output_image\n", argv[0]);
        return 0;
    }

    string input_fname(argv[1]);
    string output_fname(argv[2]);

    Mat image = imread(input_fname);
    imwrite(output_fname, image);

    return 0;
}
