#include <utility>
#include <memory>
#include "tty_io.hpp"
#include "video_capture_source.hpp"
#include "http_request_sink.hpp"
#include "packet.hpp"
#include "tty_io.hpp"
#include "timer.hpp"
#include "imshow_sink.hpp"
#include <chrono>
#include "mark_face.hpp"
#include "memory/bytes.hpp"
#include <signal.h>
#include "memory/encoding.hpp"
#include "bt.hpp"
#include "pipeline_core.hpp"
#include "video_capture_source.hpp"
#include "pipe_encode.hpp"
#include "pipe_context.hpp"
#include "packet_demux.hpp"
#include "data_pipe.hpp"
#include "pipe_lambda.hpp"
#include "fdfr_version.h"

using namespace std;

constexpr size_t buffer_frame = 3;

TtyConfig InitTtyConfig() {
    TtyConfig config;
    config.device = "/dev/ttyACM0";
    config.baud = B4000000;

    return config;
}

auto tty = make_shared<TtyIO>(InitTtyConfig());

int main(int argc, char* argv[]) {
    google::InitGoogleLogging(argv[0]);
    FDFRVersion::dump_version();

    signal(SIGINT, [] (int sig) -> void { tty->close(); });
    signal(SIGSEGV, [] (int sig) -> void { print_backtrace(); });
    signal(SIGABRT, [] (int sig) -> void { print_backtrace(); });

    auto demux = make_shared<PacketDemux>(tty);
    auto frame_pipe = make_shared<DataPipe<image_t>>(3, data_pipe_mode::dropping);
    auto video_pipe = pipeline::make(
        pipeline::wrap_source(make_shared<VideoCaptureSource>(0)),
        [](const image_t &img) -> cv::Mat {
            return cv::Mat(img).clone();
        },
        make_shared<pipeline::Encode<image_t>>(),
        pipeline::wrap_sink(demux->create_stream("video")),
        [](cv::Mat &&img) -> image_t {
            return image_t(move(img));
        },
        pipeline::wrap_sink(frame_pipe)
    );

    auto face_pipe = pipeline::make(
        pipeline::wrap_source(demux->create_stream("face")),
        make_shared<pipeline::Decode<memory::Iterable<face_info_t>, memory::Iterable<fr_name_t>>>(),
        pipeline::wrap_source(frame_pipe),
        make_shared<MarkFace<memory::Iterable<face_info_t>, memory::Iterable<fr_name_t>>>(),
        pipeline::wrap_sink(make_shared<ImshowSink>())
    );

    demux->start();

    thread video_thread([&video_pipe]() {
        video_pipe->execute();
        fprintf(stderr, "video terminating\n");
        video_pipe.reset();
    });
    face_pipe->execute();
    fprintf(stderr, "face terminating\n");
    face_pipe.reset();

    video_thread.join();

    return 0;
}
