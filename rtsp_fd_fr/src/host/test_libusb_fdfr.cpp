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
#include <cstdlib>
#include "usb_comm.hpp"
#include "pipe_executor.hpp"
#include "http_request_sink.hpp"
#include "image_file_source.hpp"

using namespace std;

constexpr size_t buffer_frame = 3;

auto usb = make_shared<usb_io::Protocol>();

int main(int argc, char* argv[]) {
    google::InitGoogleLogging(argv[0]);
    FDFRVersion::dump_version();

    srand(time(NULL));

    signal(SIGINT, [] (int sig) -> void { usb->close(); });
    signal(SIGSEGV, [] (int sig) -> void { print_backtrace(); usb->close(); });
    signal(SIGABRT, [] (int sig) -> void { print_backtrace(); usb->close(); });
    fprintf(stderr, "launching\n");
    queue<chrono::high_resolution_clock::time_point> tps;
    auto throughput_control = make_shared<DataPipe<>>(buffer_frame, data_pipe_mode::blocking);
    auto img_src_seq = pipeline::make_sequence(
        pipeline::wrap_sink(throughput_control),
        pipeline::wrap_source(make_shared<VideoCaptureSource>(0)),
        make_shared<pipeline::Encode<image_t>>()
    );
    auto result_output_seq = pipeline::make_sequence(
        pipeline::wrap_source(throughput_control),
        make_shared<pipeline::Decode<image_t, memory::Iterable<face_info_t>, memory::Iterable<fr_name_t>>>(),
        make_shared<MarkFace<memory::Iterable<face_info_t>, memory::Iterable<fr_name_t>>>(),
        pipeline::wrap_sink(make_shared<ImshowSink>())
    );

    while (usb->is_opened()) {
        list<chrono::high_resolution_clock::time_point> frame_tps;
        auto usb_session = usb->get_session();
        auto img_pipe = pipeline::make(
            img_src_seq,
            pipeline::wrap_sink(usb_session)
        );
        auto result_pipe = pipeline::make(
            pipeline::wrap_source(usb_session),
            [&frame_tps]() {
                frame_tps.push_back(chrono::high_resolution_clock::now());
                if (frame_tps.size() > 1) {
                    LOGI << "FPS: " << (frame_tps.size() - 1) * 1e+6 / chrono::duration_cast<chrono::microseconds>(frame_tps.back() - frame_tps.front()).count();
                }
                while (!frame_tps.empty() && (frame_tps.front() + chrono::seconds(5) < frame_tps.back())) {
                    frame_tps.pop_front();
                }
            },
            result_output_seq
        );

        pipeline::Executor exec;
        exec.add_pipes({img_pipe, result_pipe});
        exec.start();
        exec.join();
        break;
    }

    return 0;
}
