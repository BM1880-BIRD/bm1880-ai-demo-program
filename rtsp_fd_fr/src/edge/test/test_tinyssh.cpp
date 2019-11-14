

#include <opencv2/opencv.hpp>
#include "fdfr.hpp"
#include <string>
#include "bt.hpp"

using namespace std;
using namespace cv;

static vector<vector<string>> models = {
    {
        "/usr/data/bmodel/tiny_ssh.bmodel",
        "/usr/data/bmodel/det3.bmodel",
    },
    {
        "/usr/data/bmodel/bmface.bmodel",
    }
};

int main(int argc, char* argv[]) {
    signal(SIGSEGV, [] (int sig) -> void { print_backtrace(); });
    signal(SIGABRT, [] (int sig) -> void { print_backtrace(); });

    if (argc != 3 || strlen(argv[2]) != 1) {
        printf("usage: %s image_filename expected_face_number\n", argv[0]);
        return 0;
    }

    string fname(argv[1]);
    FDFR fdfr(FD_TINYSSH, models[0], FR_BMFACE, models[1]);
    vector<qnn::vision::face_info_t> face_infos;

    Mat image = imread(fname);
    fdfr.detect(image, face_infos);

    int expect_num = atoi(argv[2]);

    printf("expect %d faces got %ld faces\n", expect_num, face_infos.size());
    if (face_infos.size() == expect_num) {
        printf("PASS\n");
    } else {
        printf("FAILED\n");
    }
    return 0;
}
