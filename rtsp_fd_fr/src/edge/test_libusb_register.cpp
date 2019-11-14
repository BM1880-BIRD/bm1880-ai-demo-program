#include <utility>
#include <memory>
#include "tty_io.hpp"
#include "video_capture_source.hpp"
#include "mark_face_sink.hpp"
#include <opencv2/opencv.hpp>
#include "fdfr.hpp"
#include "http_request_sink.hpp"
#include "packet.hpp"
#include "tty_io.hpp"
#include "repository.hpp"
#include "memory/bytes.hpp"
#include "face_impl.hpp"
#include "timer.hpp"
#include <signal.h>
#include "face_pipe.hpp"
#include "config_file.hpp"
#include "pipe_encode.hpp"
#include "pipe_context.hpp"
#include "pipe_context_reader.hpp"
#include "pipe_lambda.hpp"
#include "bt.hpp"
#include "packet_demux.hpp"
#include "pipeline_core.hpp"
#include "qnn.hpp"
#include "fdfr_version.h"
#include "usb_comm.hpp"

using namespace std;

shared_ptr<usb_io::Protocol> usb;

int main(int argc, char *argv[]) {
    google::InitGoogleLogging(argv[0]);
    FDFRVersion::dump_version();

    signal(SIGINT, [] (int sig) -> void { LOGI << "closing usb"; usb->close(); });
    signal(SIGSEGV, [] (int sig) -> void { print_backtrace(); LOGI << "closing usb"; usb->close();});
    signal(SIGABRT, [] (int sig) -> void { print_backtrace(); LOGI << "closing usb"; usb->close();});

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <config_file_path> <name>\n", argv[0]);
        return 0;
    }

    ConfigFile config;
    try {
        config.setConfigPath(argv[1]);
        config.reload();
    } catch (ios_base::failure &e) {
        fprintf(stderr, "failed reading config file %s: %s\n", argv[1], e.what());
        exit(0);
    }
    config.print();
    shared_ptr<Repository> repo;
    try {
        repo = make_shared<Repository>(config.get<string>("repository"));
    } catch (ios_base::failure &e) {
        fprintf(stderr, "failed initializing repository %s: %s\n", config.get<string>("repository"), e.what());
        exit(0);
    }

    auto context = make_shared<pipeline::Context>();
    context->set<string>("command", "faceid");

    try {
        usb = make_shared<usb_io::Protocol>();
    } catch (ios_base::failure &e) {
        fprintf(stderr, "failed initializing usb_io: %s\n", e.what());
        exit(0);
    }

    try {
        auto core_seq = pipeline::make_sequence(
            make_shared<pipeline::Decode<image_t>>(),
            make_shared<face::pipeline::FD>(config.get<face_algorithm_t>("detector"), config.get<vector<string>>("detector_models")),
            make_shared<face::pipeline::FR>(config.get<face_algorithm_t>("extractor"), config.get<vector<string>>("extractor_models")),
            make_shared<face::pipeline::RegisterNNM>(argv[2], repo),
            make_shared<pipeline::Encode<image_t, vector<face_info_t>, vector<fr_name_t>>>()
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
    } catch (ios_base::failure &e) {
        fprintf(stderr, "failed running pipeline: %s\n", e.what());
        exit(0);
    }

    fprintf(stderr, "terminating\n");

    return 0;
}
