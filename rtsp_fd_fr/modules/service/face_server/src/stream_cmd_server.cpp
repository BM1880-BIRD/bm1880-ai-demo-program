#include "stream_cmd_server.hpp"
#include <opencv2/opencv.hpp>
#include <memory>
#include "repository.hpp"
#include "memory/bytes.hpp"
#include "face_api.hpp"
#include "face_pipe.hpp"
#include "thread_runner.hpp"
#include "mark_face.hpp"
#include "capture_stream.hpp"
#include "pipe_encode.hpp"
#include "pipe_context_reader.hpp"
#include "pipe_lambda.hpp"
#include "pipe_console.hpp"
#include "http_request_sink.hpp"
#include "face_antispoofing_pipe.hpp"
#include "pipe_image_concat.hpp"
#include "data_pipe.hpp"
#include "image_http_encode.hpp"

using namespace std;

TtyConfig InitTtyConfig() {
    TtyConfig config;
    config.device = "/dev/ttyGS0";
    config.baud = B4000000;

    return config;
}

inline bool FileExist(const string& name) {
    ifstream f(name.c_str());
    return f.good();
}

StreamCmdServer::StreamCmdServer() : StreamCmdServer("server.conf") {}

StreamCmdServer::StreamCmdServer(string &&config_path) : config_path_(move(config_path)), async_capacity_(5), running_flag_(true) {
    tty_config_ = InitTtyConfig();
    tty_ = make_shared<TtyIO>(tty_config_);
    context_ = make_shared<pipeline::Context>();
    cmd_switch_ = make_shared<pipeline::DynamicSwitch<cmd_switch_tag_t, std::tuple<std::string &&>, std::tuple<std::string>>>();
}

StreamCmdServer::~StreamCmdServer() {
    stop();
}

void StreamCmdServer::init() {
    // Load config and setup context
    if (!FileExist(config_path_)) {
        cout << "[Error] config file not exist." << endl;
    }
    config_.setConfigPath(config_path_);
    config_.reload();

    cout << "[Info] Server [ver:" << config_.get<string>(CONFIG_KEY_SERVER_VERSION) << "] run ..." << endl;

    // Setup context
    context_->set<string>(CONTEXT_KEY_SERVER_VERSION, config_.get<string>(CONFIG_KEY_SERVER_VERSION));
    context_->set<string>(CONTEXT_KEY_STREAM_CMD, "faceid");

    // Init fdfr
    repo_ = make_shared<Repository>(config_.get<string>(CONFIG_KEY_REPO));

    if (nullptr == demux_) {
        demux_ = make_shared<PacketDemux>(tty_);
    }
}

void StreamCmdServer::startStreamPipeline(const string &stream_name) {
    // Check stream enable
    if (0 == config_.get<string>(stream_name, CONFIG_KEY_STM_DISABLE).compare("true")) {
        cout << "[Info] " << stream_name << " disabled." << endl;
        return;
    }

    // Handle source type
    if (0 == config_.get<string>(stream_name, CONFIG_KEY_STM_SRC_TYPE).compare(CONFIG_KEY_STM_SRC_DEVICE)) {
        // Device source (uvc camera)
        int device_source = config_.get<int>(stream_name, CONFIG_KEY_STM_SRC_DEVICE_IDX);

        cout << "[Info] " << stream_name << " device_source:" << device_source << endl;
        context_->set<int>(stream_name + "_device_source", move(device_source));
    } else {
        // Rtsp source
        string rtsp_url = "rtsp://" + config_.get<string>(stream_name, CONFIG_KEY_STM_SRC_RTSP_HOST) + ":" +
                                      config_.get<string>(stream_name, CONFIG_KEY_STM_SRC_RTSP_PORT) + "/" +
                                      config_.get<string>(stream_name, CONFIG_KEY_STM_SRC_RTSP_URL);

        cout << "[Info] " << stream_name << " rtsp_url:" << rtsp_url << endl;
        context_->set<string>(stream_name + "_rtsp_url", string(rtsp_url));
    }

    auto image_data_pipe = std::make_shared<DataPipe<image_t>>(async_capacity_, data_pipe_mode::dropping);
    auto face_data_pipe = std::make_shared<DataPipe<image_t, vector<fd_result_t>, vector<fr_name_t>, vector<bool>>>(async_capacity_, data_pipe_mode::dropping);

    auto input_pipe = pipeline::make(
        // Get stream source type (device, rtsp)
        [this, stream_name]() -> tuple<string> {
            return {config_.get<string>(stream_name, CONFIG_KEY_STM_SRC_TYPE)};
        },
        pipeline::make_switch<std::string>(
            make_pair(
                CONFIG_KEY_STM_SRC_RTSP,
                make_shared<CaptureStream<string>>(context_->get<string>(stream_name + "_rtsp_url"))
            ),
            make_pair(
                pipeline::switch_default(),
                make_shared<CaptureStream<int>>(context_->get<int>(stream_name + "_device_source"))
            )
        ),
        pipeline::wrap_sink(image_data_pipe)
    );

    auto output_pipe = pipeline::make(
        // Prepend stream destination type (http, tty)
        [this, stream_name]() -> tuple<string> {
            return {config_.get<string>(stream_name, CONFIG_KEY_STM_DEST_TYPE)};
        },
        pipeline::wrap_source(face_data_pipe),
        pipeline::make_switch<std::string>(
            // Sink to tty
            make_pair(string(CONFIG_KEY_STM_DEST_TTY), pipeline::make_sequence(
                make_shared<pipeline::Encode<vector<face_info_t>>>(),
                pipeline::wrap_sink(demux_->create_stream(stream_name)),
                // Set use demux flag
                [this]() -> tuple<> {
                    context_->set<bool>(CONTEXT_KEY_USE_DEMUX, true);
                    return {};
                }
            )),
            // Sink to http
            make_pair(string(CONFIG_KEY_STM_DEST_HTTP), pipeline::make_sequence(
                make_shared<MarkFace<vector<face_info_t>, vector<fr_name_t>>>(),
                pipeline::wrap_sink(make_shared<HttpRequestSink<image_t>>(
                    config_.get<string>(stream_name, CONFIG_KEY_STM_DEST_HTTP_HOST),
                    config_.get<int>(stream_name, CONFIG_KEY_STM_DEST_HTTP_PORT),
                    config_.get<string>(stream_name, CONFIG_KEY_STM_DEST_HTTP_URL)
                ))
            )),
            // Sink to http with faces
            make_pair(string(CONFIG_KEY_STM_DEST_HTTP_WITH_FACES),
                pipeline::make_sequence(
                    [](const image_t &img) -> cv::Mat {
                        return cv::Mat(img).clone();
                    },
                    make_shared<MarkFace<vector<face_info_t>, vector<fr_name_t>>>(),
                    pipeline::wrap_sink(make_shared<HttpRequestSink<image_t>>(
                        config_.get<string>(stream_name, CONFIG_KEY_STM_DEST_HTTP_HOST),
                        config_.get<int>(stream_name, CONFIG_KEY_STM_DEST_HTTP_PORT),
                        config_.get<string>(stream_name, CONFIG_KEY_STM_DEST_HTTP_URL)
                    )),
                    [](cv::Mat &&img) -> image_t {
                        return img;
                    },
                    // Send real faces
                    make_shared<face::pipeline::FaceCrop<cv::Mat>>(),
                    [](vector<cv::Mat> &&images, const vector<bool> &is_real_faces) {
                        // Check real faces
                        vector<cv::Mat> real_face_images;
                        for (int i = 0; i < is_real_faces.size(); i++) {
                            if (is_real_faces[i]) {
                                real_face_images.emplace_back(images[i]);
                            }
                        }

                        return make_tuple(move(real_face_images));
                    },
                    [](image_t &&) -> tuple<> {
                        return {};
                    },
                    make_shared<pipeline::ImageConcat>(),
                    [](const image_t &image) -> tuple<bool> {
                        return !image.empty();
                    },
                    pipeline::make_switch<bool>(
                        make_pair(
                            true,
                            pipeline::wrap_sink(make_shared<HttpRequestSink<image_t>>(
                                config_.get<string>(stream_name, CONFIG_KEY_STM_DEST_HTTP_HOST),
                                config_.get<int>(stream_name, CONFIG_KEY_STM_DEST_HTTP_PORT),
                                config_.get<string>(stream_name, CONFIG_KEY_STM_DEST_HTTP_FACES_URL)
                            ))
                        )
                    )
                )
            ),
            // Default sink to tty
            make_pair(pipeline::switch_default(), pipeline::make_sequence(
                make_shared<pipeline::Encode<vector<face_info_t>>>(),
                pipeline::wrap_sink(demux_->create_stream(stream_name)),
                // Set use demux flag
                [this]() -> tuple<> {
                    context_->set<bool>(CONTEXT_KEY_USE_DEMUX, true);
                    return {};
                }
            ))
        )
    );

    // Stream process command switch
    auto stream_switch = pipeline::make_switch<string>(
        make_pair(
            string("faceid"),
            make_shared<face::pipeline::Identify>(config_.get<double>("similarity_threshold"), repo_)
        ),
        make_pair(
            string("face"),
            [](const std::vector<face_info_t> &infos) {
                return make_tuple(std::vector<fr_name_t>(infos.size(), {{}}));
            }
        )
    );

    // Stream face anti-spoofing switch
    auto stream_antispoofing_switch = pipeline::make_switch<bool>(
        make_pair(
            true,
            pipeline::make_sequence(
                make_shared<face::pipeline::FaceAntiSpoofing>(config_.get<vector<string>>(stream_name, CONFIG_KEY_STM_FACE_ANTI_SPOOFING_MODELS),
                                                              config_.get<bool>(stream_name, CONFIG_KEY_STM_FACE_ANTI_SPOOFING_LABEL))
            )
        ),
        // Make dummy output
        make_pair(
            pipeline::switch_default(),
            [](const std::vector<face_info_t> &infos) {
                vector<bool> is_real_faces(infos.size(), true);
                cv::Mat depth_face_mat_dummy;
                return make_tuple(is_real_faces, depth_face_mat_dummy);
            }
        )
    );

    auto core_pipe = pipeline::make(
        pipeline::wrap_source(image_data_pipe),
        make_shared<face::pipeline::FD>(config_.get<face_algorithm_t>(CONFIG_KEY_DETECTOR), config_.get<vector<string>>(CONFIG_KEY_DETECTOR_MODEL)),
        // Check need face anti-spoofing
        [this, stream_name]() -> tuple<bool> {
            return config_.get<bool>(stream_name, CONFIG_KEY_STM_FACE_ANTI_SPOOFING);
        },
        stream_antispoofing_switch,
        make_shared<face::pipeline::FR>(config_.get<face_algorithm_t>(CONFIG_KEY_EXTRACTOR), config_.get<vector<string>>(CONFIG_KEY_EXTRACTOR_MODEL)),
        [this]() -> tuple<string> {
            return {context_->get_default<string>(CONTEXT_KEY_STREAM_CMD, "faceid")->value};
        },
        stream_switch,
        pipeline::wrap_sink(face_data_pipe)
    );

    // Create thread to run stream pipeline
    shared_ptr<ConditionalRunner> stream_thead_p = make_shared<ConditionalRunner>();
    stream_thead_p->add_task([core_pipe]() {
        core_pipe->execute_once();
    });
    stream_thead_p->add_task([input_pipe]() {
        input_pipe->execute_once();
    });
    stream_thead_p->add_task([output_pipe]() {
        output_pipe->execute_once();
    });
    if (0 != config_.get<string>(stream_name, CONFIG_KEY_STM_AUTO_START).compare("false")) {
        stream_thead_p->start();
    }

    stream_thread_map_[stream_name] = stream_thead_p;

    // Add stream command
    if (0 == config_.get<string>(stream_name, CONFIG_KEY_STM_SRC_TYPE).compare(CONFIG_KEY_STM_SRC_DEVICE)) {
        addCmd("set_" + stream_name + "_source", [this, stream_name](tuple<string> &&value) -> tuple<string> {
            int source = stoi(get<0>(value));
            setContextValue<int>(stream_name + "_source", source);
            return make_tuple("Set " + stream_name + " source to " + get<0>(value));
        }, "Set " + stream_name + " source for video device index. ex: " + "set_" + stream_name + "_source 0");

        addCmd("get_" + stream_name + "_source", [this, stream_name](tuple<string> &&value) -> tuple<string> {
            string source = to_string(getContextValue<int>(stream_name + "_source"));
            return make_tuple(source);
        }, "Get " + stream_name + " video device index.");
    } else {
        addCmd("set_" + stream_name + "_rtsp_url", [this, stream_name](tuple<string> &&value) -> tuple<string> {
            setContextValue<string>(stream_name + "_rtsp_url", get<0>(value));
            return make_tuple("Set " + stream_name + " rtsp url to " + get<0>(value));
        }, "Set " + stream_name + " rtsp url. ex: " + "set_" + stream_name + "_rtsp_url rtsp://0.0.0.0/test");

        addCmd("get_" + stream_name + "_rtsp_url", [this, stream_name](tuple<string> &&value) -> tuple<string> {
            return make_tuple(getContextValue<string>(stream_name + "_rtsp_url"));
        }, "Get " + stream_name + " rtsp url.");
    }
}

void StreamCmdServer::startCommandPipeline() {
    if ((!FileExist(tty_config_.device)) || (0 == config_.get<string>(CONFIG_KEY_CMD_STM_DISABLE).compare("true"))) {
        return;
    }

    cout << "[Info] command stream enabled." << endl;
    initDefaultCmd();

    // Set use demux flag
    context_->set<bool>(CONTEXT_KEY_USE_DEMUX, true);

    // Setup command pipeline
    auto cmd_pipe = pipeline::make(
        pipeline::wrap_source(demux_->create_stream(STREAM_CMD)),
        make_shared<pipeline::Decode<string>>(),
        make_shared<ConsoleOutPipeComp<string>>("[Info] Receive command:"),
        [this](string &&data) -> tuple<cmd_switch_tag_t, string> {
            return splitStrCmdValue(move(data));
        },
        cmd_switch_,
        make_shared<pipeline::Encode<string>>(),
        pipeline::wrap_sink(demux_->create_stream(STREAM_CMD_RESP))
    );

    cmd_pipe->set_context(context_);
    cmd_thread_.start([cmd_pipe, this]() {
        cmd_pipe->execute_once();
    });
}

void StreamCmdServer::run() {
    init();

    // Save streams name
    string streams = "";
    // Get stream sections from config file
    vector<string> sections_name = config_.getSectionsName();
    for (string section_name : sections_name) {
        if (0 != section_name.find(CONFIG_KEY_STM_PREFIX)) {
            continue;
        }

        streams = (streams.empty()) ? section_name : streams + "," + section_name;
        // Setup stream pipeline
        startStreamPipeline(section_name);
    }
    context_->set<string>(CONTEXT_KEY_STREAMS, streams.data());

    // Setup command pipeline
    startCommandPipeline();

    // Check need start demux or not
    if (context_->get_default<bool>(CONTEXT_KEY_USE_DEMUX, false)->value) {
        demux_->start();
    }

    // Do main loop
    while (running_flag_) {
        mainLoop();
    }

    stopServer();
}

void StreamCmdServer::mainLoop() {
    // Do nothing
    usleep(500000);
}

void StreamCmdServer::stop() {
    cout << "[Info] Server stop ..." << endl;
    running_flag_ = false;
}

void StreamCmdServer::stopServer() {
    tty_->close();
    cmd_thread_.stop();
    cmd_thread_.join();
    for (auto &pair : stream_thread_map_) {
        auto thread = pair.second;
        thread->stop();
        thread->join();
    }
}

void StreamCmdServer::setConfigPath(const std::string &config_path) {
    config_path_ = move(config_path);
}

void StreamCmdServer::initDefaultCmd() {
    addCmd(CMD_GET_SERVER_VERSION, [this](tuple<string>) -> tuple<string> {
        return {context_->get_default<string>(CONFIG_KEY_SERVER_VERSION, "")->value};
    }, "get server's version.");

    addCmd(CMD_GET_STREAMS, [this](tuple<string>) -> tuple<string> {
        return {context_->get_default<string>(CONTEXT_KEY_STREAMS, "")->value};
    }, "get server's streams name.");

    addCmd(CMD_START_STREAMS, [this](tuple<string> &&value) -> tuple<string> {
        string stream_name = get<0>(value);
        string resp = "start " + stream_name + " failed";
        if (stream_thread_map_.end() != stream_thread_map_.find(stream_name)) {
            auto stream_thread = stream_thread_map_[stream_name];
            stream_thread->start();
            resp = "start " + stream_name + " success";
        }

        return make_tuple(resp);
    }, "start streams. ex: start_stream stream_1");

    addCmd(CMD_PAUSE_STREAMS, [this](tuple<string> &&value) -> tuple<string> {
        string stream_name = get<0>(value);
        string resp = "pause " + stream_name + " failed";
        if (stream_thread_map_.end() != stream_thread_map_.find(stream_name)) {
            auto stream_thread = stream_thread_map_[stream_name];
            stream_thread->pause();
            resp = "pause " + stream_name + " success";
        }

        return make_tuple(resp);
    }, "pause streams. ex: pause_stream stream_1");

    addCmd(CMD_STOP_STREAMS, [this](tuple<string> &&value) -> tuple<string> {
        string stream_name = get<0>(value);
        string resp = "stop " + stream_name + " failed";
        if (stream_thread_map_.end() != stream_thread_map_.find(stream_name)) {
            auto stream_thread = stream_thread_map_[stream_name];
            stream_thread->stop();
            resp = "stop " + stream_name + " success";
        }

        return make_tuple(resp);
    }, "stop streams. ex: stop_stream stream_1");

    addCmd(CMD_STOP_SERVER, [this](tuple<string>) -> tuple<string> {
        stop();
        return make_tuple("Stop server success.");
    }, "stop server.");

    addCmd(CMD_REGISTER_FACE, [this](tuple<string> &&name_tuple) -> tuple<string> {
        string name = get<0>(name_tuple);
        // Command response string
        string response = "Register " + name + " face failed";
        // Get face data from demux and register face
        auto register_face_pipe = pipeline::make(
            pipeline::wrap_source(demux_->create_stream(STREAM_REGISTER_FACE)),
            make_shared<pipeline::Decode<image_t>>(),
            // Combine name and mat
            [name](image_t &&mat) -> tuple<string, image_t> {
                return {name, mat};
            },
            make_shared<face::pipeline::FD>(config_.get<face_algorithm_t>(CONFIG_KEY_DETECTOR), config_.get<vector<string>>(CONFIG_KEY_DETECTOR_MODEL)),
            make_shared<face::pipeline::FR>(config_.get<face_algorithm_t>(CONFIG_KEY_EXTRACTOR), config_.get<vector<string>>(CONFIG_KEY_EXTRACTOR_MODEL)),
            make_shared<face::pipeline::Register>(repo_),
            [name, &response](face::pipeline::Register::result_t &&success) -> tuple<> {
                // If success, update command response
                if (success) {
                    response = "Register " + name + " face success";
                }
            }
        );

        register_face_pipe->execute_once();

        return make_tuple(response);
    }, "register face. ex: register_face name");

    addCmd(CMD_GET_CMDS, [this](tuple<string>) -> tuple<string> {
        string all_cmd_desp = "";
        for (auto &pair : cmd_map_) {
            string desp = pair.second;
            all_cmd_desp += " - " + pair.first;
            if (!desp.empty()) {
                all_cmd_desp += " : " + pair.second;
            }
            all_cmd_desp += "\n";
        }

        return make_tuple(all_cmd_desp);
    }, "get all commands.");

    cmd_switch_->set_default_command([this](tuple<string>) -> tuple<string> {
        return make_tuple(CMD_RESP_UNKNOWN);
    });
}

void StreamCmdServer::addCmd(const string &cmd, function<tuple<string>(tuple<string>)> f, std::string &&cmd_desp) {
    cmd_switch_->set_command(cmd, f);
    cmd_map_[cmd] = cmd_desp;
}

void StreamCmdServer::executeCmd(const std::string &cmd, const std::string &value, std::string &response)
{
    auto cmd_pipe = pipeline::make(
        [cmd, value]() -> tuple<cmd_switch_tag_t, string> {
            return {cmd, value};
        },
        cmd_switch_,
        [&response](string &&cmd_response) -> tuple<> {
            response = cmd_response;
            return {};
        }
    );

    cmd_pipe->set_context(context_);
    cmd_pipe->execute_once();
}

void StreamCmdServer::setAsyncCapacity(const size_t capacity) {
    // Async queue size
    async_capacity_ = capacity;
}

bool StreamCmdServer::registerFace(const std::string &name, cv::Mat &&image) {
    bool register_success = false;

    auto register_face_pipe = pipeline::make(
        [name, image]() -> tuple<string, image_t> {
            return {name, image};
        },
        make_shared<face::pipeline::FD>(config_.get<face_algorithm_t>(CONFIG_KEY_DETECTOR), config_.get<vector<string>>(CONFIG_KEY_DETECTOR_MODEL)),
        make_shared<face::pipeline::FR>(config_.get<face_algorithm_t>(CONFIG_KEY_EXTRACTOR), config_.get<vector<string>>(CONFIG_KEY_EXTRACTOR_MODEL)),
        make_shared<face::pipeline::Register>(repo_),
        [name, &register_success](face::pipeline::Register::result_t &&success) -> tuple<> {
            register_success = success;
        }
    );

    register_face_pipe->execute_once();

    return register_success;
}

tuple<string, string> StreamCmdServer::splitStrCmdValue(string &&str) {
    string cmd, value;
    // Find space for split cmd and value
    size_t space_idx = str.find(" ");
    if (space_idx != string::npos) {
        cmd = str.substr(0, space_idx);
        value = str.substr(space_idx + 1);
    } else {
        cmd = move(str);
        value = "";
    }

    return {cmd, value};
}
