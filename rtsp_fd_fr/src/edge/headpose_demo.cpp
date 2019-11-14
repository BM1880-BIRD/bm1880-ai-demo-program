#include <utility>
#include <memory>
#include "tty_io.hpp"
#include "video_capture_source.hpp"
#include <opencv2/opencv.hpp>
#include "http_request_sink.hpp"
#include "packet.hpp"
#include "tty_io.hpp"
#include "memory/bytes.hpp"
#include "face_impl.hpp"
#include "timer.hpp"
#include <signal.h>
#include "face_pipe.hpp"
#include "config_file.hpp"
#include "pipeline_core.hpp"
#include "pipe_encode.hpp"
#include "pipe_context.hpp"
#include "pipe_context_reader.hpp"
#include "pipe_lambda.hpp"
#include "bt.hpp"
#include "packet_demux.hpp"
#include "pipeline_core.hpp"
#include "qnn.hpp"
#include "face_align_pipe.hpp"
#include "face_pose_pipe.hpp"
#include "fdfr_version.h"

using namespace std;

TtyConfig InitTtyConfig() {
    TtyConfig config;
    config.device = "/dev/ttyGS0";
    config.baud = B4000000;

    return config;
}

auto tty = make_shared<TtyIO>(InitTtyConfig());

int main(int argc, char *argv[]) {
    google::InitGoogleLogging(argv[0]);
    FDFRVersion::dump_version();

    signal(SIGINT, [] (int sig) -> void { tty->close(); });
    signal(SIGSEGV, [] (int sig) -> void { print_backtrace(); });
    signal(SIGABRT, [] (int sig) -> void { print_backtrace(); });

    if (argc != 2) {
        fprintf(stderr, "Usage: pipeline_1880_test <config_file_path>\n");
        return 0;
    }

    ConfigFile config(argv[1]);
    config.print();

    auto context = make_shared<pipeline::Context>();
    context->set<string>("command", "faceid");
    auto demux = make_shared<PacketDemux>(tty);

    auto pipe = pipeline::make(
        pipeline::wrap_source(demux->create_stream("video")),
        make_shared<pipeline::Decode<image_t>>(),
        make_shared<face::pipeline::FD>(
            config.get<face_algorithm_t>("detector"), config.get<vector<string>>("detector_models")
        ),
        make_shared<face::pipeline::FaceCrop<cv::Mat>>(),
        make_shared<face::pipeline::Facepose>(config.get<string>("facepose_model")),
        [](vector<array<float, 3>> &&angles_list) -> tuple<vector<fr_name_t>> {
            vector<fr_name_t> prints;
            prints.reserve(angles_list.size());

            for (auto &angles : angles_list) {
                prints.emplace_back();
                sprintf(prints.back().data(), "(%c%02d, %c%02d, %c%02d)",
                        angles[0] > 0 ? '+' : '-', (int)abs(round(angles[0])),
                        angles[1] > 0 ? '+' : '-', (int)abs(round(angles[1])),
                        angles[2] > 0 ? '+' : '-', (int)abs(round(angles[2])));
            }
            return make_tuple(move(prints));
        },
        make_shared<pipeline::Encode<vector<face_info_t>, vector<fr_name_t>>>(),
        pipeline::wrap_sink(demux->create_stream("face"))
    );

    demux->start();
    pipe->set_context(context);
    pipe->execute();
    fprintf(stderr, "terminating\n");

    return 0;
}
