#include "stream_cmd_client.hpp"
#include "imshow_sink.hpp"
#include "pipe_encode.hpp"
#include "pipe_lambda.hpp"
#include "pipe_console.hpp"
#include "face_api.hpp"
#include "video_capture_source.hpp"
#include "image.hpp"

using namespace std;

TtyConfig InitTtyConfig() {
    TtyConfig config;
    config.device = "/dev/ttyACM0";
    config.baud = B4000000;

    return config;
}

StreamCmdClient::StreamCmdClient() {
    tty_ = make_shared<TtyIO>(InitTtyConfig());
    demux_ = make_shared<PacketDemux>(tty_);
    context_ = make_shared<pipeline::Context>();
    running_flag = true;
}

StreamCmdClient::~StreamCmdClient() {
    stop();
}

void StreamCmdClient::startStreamPipeline(const string &stream_name) {
    auto stream_pipe = pipeline::make(
        pipeline::wrap_source(demux_->create_stream(stream_name)),
        make_shared<pipeline::Decode<memory::Iterable<face_info_t>>>()
        // make_shared<pipeline::Decode<cv::Mat>>(),
        // pipeline::wrap_sink((make_shared<ImshowSink>(stream_name)))
    );

    shared_ptr<ThreadRunner> stream_thead_p = make_shared<ThreadRunner>();
    stream_thead_p->start([stream_pipe]() {
        stream_pipe->execute_once();
    });
    stream_thread_map_[stream_name] = stream_thead_p;
}

void StreamCmdClient::run() {
    for (string stream_name : stream_vec_) {
        // Setup streams pipeline
        startStreamPipeline(stream_name);
    }

    // Setup command pipeline
    auto cmd_pipe = pipeline::make(
        // Wait console input
        pipeline::wrap_source(make_shared<ConsoleCaptureSource>()),
        // Keep command
        [this](string &&cmd) -> tuple<string> {
            context_->set<string>("cmd", string(cmd).data());
            return make_tuple(cmd);
        },
        make_shared<pipeline::Encode<string>>(),
        pipeline::wrap_sink(demux_->create_stream(STREAM_CMD)),
        // Handle special command
        [this]() -> tuple<> {
            string current_cmd = context_->get_default<string>("cmd", "")->value;
            // Register face will capture image from camera and send to server
            if (0 == current_cmd.find(CMD_REGISTER_FACE)) {
                auto send_face_data = pipeline::make(
                    pipeline::wrap_source(make_shared<VideoCaptureSource>(0)),
                    make_shared<pipeline::Encode<image_t>>(),
                    pipeline::wrap_sink(demux_->create_stream(STREAM_REGISTER_FACE))
                );

                send_face_data->execute_once();
            }

            return make_tuple();
        },
        // Receive response
        pipeline::wrap_source(demux_->create_stream(STREAM_CMD_RESP)),
        make_shared<pipeline::Decode<string>>(),
        make_shared<ConsoleOutPipeComp<string>>("[Info] Receive command response:")
    );

    demux_->start();

    // Run command pipeline
    while (running_flag) {
        printUsage();
        cout << "[Info] Please input command:" << endl;
        cmd_pipe->execute_once();
    }

    stop();
}

void StreamCmdClient::stop() {
    cout << "[Info] Client stop..." << endl;
    running_flag = false;
    tty_->close();
    for (auto &pair : stream_thread_map_) {
        auto thread = pair.second;
        thread->stop();
        thread->join();
    }
}

bool StreamCmdClient::sendCmd(const string &cmd, string &response) {
    auto cmd_pipe = pipeline::make(
        [cmd]() -> string {
            return cmd.data();
        },
        make_shared<pipeline::Encode<string>>(),
        pipeline::wrap_sink(demux_->create_stream(STREAM_CMD)),
        // Receive response
        pipeline::wrap_source(demux_->create_stream(STREAM_CMD_RESP)),
        make_shared<pipeline::Decode<string>>(),
        [&response](string &&resp) -> tuple<> {
            response = resp;
        }
    );

    cmd_pipe->execute_once();

    return true;
}

void StreamCmdClient::addStream(string &&stream_name) {
    stream_vec_.push_back(move(stream_name));
}

void StreamCmdClient::printUsage() {
    cout << endl << "[Info] Use " CMD_GET_CMDS " to get server's command." << endl;
}

void StreamCmdClient::setContextValue(const string &key, string &&value) {
    context_->set<string>(key, move(value));
}

string StreamCmdClient::getContextValue(const string &key) {
    return context_->get_default<string>(key, "")->value;
}
