#include <utility>
#include <memory>
#include "json.hpp"
#include "object_detection_pipe.hpp"
#include "object_detection_encode.hpp"
#include "usb_comm.hpp"
#include "signal.h"
#include "bt.hpp"
#include "pipeline_core.hpp"
#include "pipe_encode.hpp"
#include "fdfr_version.h"
#include "timer.hpp"
#include "pipe_dump.hpp"

using namespace std;

std::shared_ptr<usb_io::Protocol> usb;

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "usage: %s <model_dir> <model> <yolov2|yolov3>", argv[0]);
        return 0;
    }

    usb = make_shared<usb_io::Protocol>();

    google::InitGoogleLogging(argv[0]);
    FDFRVersion::dump_version();

    signal(SIGINT, [] (int sig) -> void { LOGI << "closing usb"; usb->close(); });
    signal(SIGSEGV, [] (int sig) -> void { print_backtrace(); LOGI << "closing usb"; usb->close();});
    signal(SIGABRT, [] (int sig) -> void { print_backtrace(); LOGI << "closing usb"; usb->close();});

    auto core_seq = pipeline::make_sequence(
        make_shared<pipeline::Decode<generic_image_t>>(),
        [](generic_image_t &&img) -> tuple<image_t, generic_image_t::FORMAT> {
            return make_tuple<image_t, generic_image_t::FORMAT>(move(img), generic_image_t::FORMAT(img.metadata.format));
        },
        make_shared<object_detection::pipeline::ObjectDetection>(json{
            {"model_dir", argv[1]},
            {"model", argv[2]},
            {"algorithm", argv[3]},
        }),
        [](image_t &&img, generic_image_t::FORMAT &&format) -> generic_image_t {
            return img.to_format(format);
        },
        make_shared<pipeline::Encode<generic_image_t, vector<qnn::vision::object_detect_rect_t>>>()
    );

    while (usb->is_opened()) {
        auto session = usb->get_session();
        auto pipe = pipeline::make(
            pipeline::wrap_source(session),
            core_seq,
            pipeline::wrap_sink(session)
        );
        LOGI << "start pipeline";
        pipe->execute();
        LOGI << "pipeline stopped";
    }

    fprintf(stderr, "terminating\n");

    return 0;
}
