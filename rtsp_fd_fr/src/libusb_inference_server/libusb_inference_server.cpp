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
#include "pipe_executor.hpp"
#include "memory/encoding.hpp"
#include "pose_detection_pipe.hpp"
#include "pipe_dump.hpp"

using namespace std;

static std::shared_ptr<usb_io::Protocol> usb;

using switch_t = pipeline::DynamicSwitch<string, tuple<usb_io::packet_t &&>, tuple<>>;
using resp_queue_t = DataPipe<usb_io::packet_t>;
auto core_switch = make_shared<switch_t>();
auto response_queue = make_shared<resp_queue_t>(0, data_pipe_mode::blocking);

static json config = {
    {
        "yolov2",
        {
            {"model_dir", "/system/data/bmodel/yolo/"},
            {"model", "yolov2_1_3_608_608.bmodel"},
            {"algorithm", "yolov2"}
        }
    },
    {
        "yolov3",
        {
            {"model_dir", "/system/data/bmodel/yolo/"},
            {"model", "yolov3_1_3_608_608.bmodel"},
            {"algorithm", "yolov3"}
        }
    },
    {
        "openpose",
        {
            {"model_path", "/system/data/bmodel/openpose_coco_128x224.bmodel"},
            {"multi_detect", 1}
        }
    },
};

template <typename Algo, typename... ExtraResp>
static void init_algorithm(const string &tag, const json &arg, shared_ptr<switch_t> core_switch, shared_ptr<resp_queue_t> response_queue) {
    core_switch->set_command(tag, pipeline::make_sequence(
        [](usb_io::packet_t &p) { LOGD << p; },
        make_shared<pipeline::Decode<generic_image_t>>(),
        [](generic_image_t &&img) {
            return make_tuple<image_t, generic_image_t::FORMAT>(move(img), generic_image_t::FORMAT(img.metadata.format));
        },
        make_shared<Algo>(arg),
        [](image_t &&img, generic_image_t::FORMAT &&format) -> generic_image_t {
            return img.to_format(format);
        },
        make_shared<pipeline::Encode<generic_image_t, ExtraResp...>>(),
        [](usb_io::packet_t &p) { LOGD << p; },
        pipeline::wrap_sink(response_queue)
    ));
}

static void init_yolov2(shared_ptr<switch_t> core_switch, shared_ptr<resp_queue_t> response_queue) {
    json args = config["yolov2"];
    init_algorithm<object_detection::pipeline::ObjectDetection, vector<qnn::vision::object_detect_rect_t>>("yolov2", args, core_switch, response_queue);
}

static void init_yolov3(shared_ptr<switch_t> core_switch, shared_ptr<resp_queue_t> response_queue) {
    json args = config["yolov3"];
    init_algorithm<object_detection::pipeline::ObjectDetection, vector<qnn::vision::object_detect_rect_t>>("yolov3", args, core_switch, response_queue);
}

static void init_openpose(shared_ptr<switch_t> core_switch, shared_ptr<resp_queue_t> response_queue) {
    json args = config["openpose"];
    init_algorithm<pose::pipeline::PoseDetection>("openpose", args, core_switch, response_queue);
}

static void init_switch(shared_ptr<switch_t> core_switch, shared_ptr<resp_queue_t> response_queue) {
    core_switch->set_command("load", [=](std::tuple<usb_io::packet_t &&> tup) -> tuple<> {
        string algorithm;
        LOGD << get<0>(tup);
        memory::Encoding<string>::decode(get<0>(tup), algorithm);
        LOGD << algorithm;
        if (algorithm == "yolov2") {
            init_yolov2(core_switch, response_queue);
        } else if (algorithm == "yolov3") {
            init_yolov3(core_switch, response_queue);
        } else if (algorithm == "openpose") {
            init_openpose(core_switch, response_queue);
        }
        return tuple<>();
    });
}

int main(int argc, char *argv[]) {
    google::InitGoogleLogging(argv[0]);
    FDFRVersion::dump_version();

    if (argc > 2) {
        fprintf(stderr, "usage: %s [json config file]\n", argv[0]);
        exit(0);
    } else if (argc == 2) {
        json input_config;
        ifstream fin(argv[1]);
        if (fin.good()) {
            try {
                fin >> input_config;
                for (auto iter = input_config.begin(); iter != input_config.end(); iter++) {
                    config[iter.key()] = iter.value();
                }
            } catch (const nlohmann::detail::exception &e) {
                fprintf(stderr, "failed parsing config file: %s\n", argv[1]);
                exit(0);
            }
            LOGD << "config: ";
            LOGD << config.dump(4);
        } else {
            fprintf(stderr, "failed reading config file: %s\n", argv[1]);
            exit(0);
        }
    } else {
        LOGD << "no config file specified. use default config";
        LOGD << config.dump(4);
    }

    usb = make_shared<usb_io::Protocol>();

    signal(SIGINT, [] (int sig) -> void { LOGI << "closing usb"; usb->close(); response_queue->close(); });
    signal(SIGSEGV, [] (int sig) -> void { print_backtrace(); LOGI << "closing usb"; usb->close();});
    signal(SIGABRT, [] (int sig) -> void { print_backtrace(); LOGI << "closing usb"; usb->close();});


    init_switch(core_switch, response_queue);

    while (usb->is_opened()) {
        auto session = usb->get_session();
        auto input_pipe = pipeline::make(
            pipeline::wrap_source(session),
            [](usb_io::packet_t &packet) {
                LOGD << packet;
            },
            make_shared<pipeline::PartialDecode<string>>(),
            [](string &command, usb_io::packet_t &packet) {
                LOGD << command << " " << packet;
            },
            core_switch
        );
        auto output_pipe = pipeline::make(
            pipeline::wrap_source(response_queue),
            pipeline::wrap_sink(session)
        );

        pipeline::Executor exec;
        exec.add_pipes({input_pipe, output_pipe});
        exec.start();
        exec.join();
    }

    fprintf(stderr, "terminating\n");

    return 0;
}
