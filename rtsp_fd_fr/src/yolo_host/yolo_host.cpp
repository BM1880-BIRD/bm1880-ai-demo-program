#include "pipe_encode.hpp"
#include "vector"
#include "pipe_source.hpp"
#include "usb_comm.hpp"
#include "imshow_sink.hpp"
#include "video_capture_source.hpp"
#include "pipe_sink.hpp"
#include"object_detection_encode.hpp"
#include <memory>
#include <signal.h>
#include "pipe_executor.hpp"
#include "data_pipe.hpp"
#include "pipeline_core.hpp"
#include "fdfr_version.h"

using namespace std;
pipeline::Executor runner;

int main(int argc, char *argv[]) {
    google::InitGoogleLogging(argv[0]);
    FDFRVersion::dump_version();

    signal(SIGINT, [] (int sig) -> void {
        runner.stop();
    });

auto s0 = pipeline::make_sequence(make_shared<pipeline::SourceWrap<VideoCaptureSource>>(0), make_shared<pipeline::Encode<image_t>>(), make_shared<pipeline::SinkWrap<usb_io::Peer>>(), make_shared<pipeline::SourceWrap<usb_io::Peer>>(), make_shared<pipeline::Decode<image_t,vector<qnn::vision::object_detect_rect_t>>>(), make_shared<pipeline::SinkWrap<ImshowSink>>("1"));
auto pl0 = pipeline::make(s0);
runner.add_pipes({pl0});

    runner.start();
    runner.join();
    return 0;
}
