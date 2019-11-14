#include "libusb_inference_client.hpp"
#include <memory>
#include <sstream>
#include "usb_comm.hpp"
#include "memory/bytes.hpp"
#include "memory/encoding.hpp"
#include "log_common.h"
#include "pipe_dump.hpp"
#include "image.hpp"

using namespace std;

namespace memory {
    template <>
    class Encoding<object_detect_rect_t> {
    public:
    /*
    typedef struct {
        float x1;
        float y1;
        float x2;
        float y2;
        int classes;
        float prob;
        std::string label;
    } object_detect_rect_t;
    */
        static std::list<Bytes> encode(object_detect_rect_t &&data) {
            std::ostringstream sstream;
            sstream << data.x1
                    << " " << data.y1
                    << " " << data.x2
                    << " " << data.y2
                    << " " << data.classes
                    << " " << data.prob
                    << " " << data.label;
            auto str = sstream.str();
            return {Bytes(std::vector<char>(str.begin(), str.end()))};
        }

        static void decode(std::list<Bytes> &list, object_detect_rect_t &output) {
            if (list.empty()) {
                throw std::out_of_range("");
            }
            Iterable<char> data(std::move(list.front()));
            list.pop_front();

            std::istringstream sstream(std::string(data.begin(), data.end()));
            sstream >> output.x1 >> output.y1 >> output.x2 >> output.y2 >> output.classes >> output.prob;
            sstream.seekg(1, std::ios_base::cur);
            std::getline(sstream, output.label);
        }
    };
} // namespace memory

static shared_ptr<usb_io::Peer> usb;

void libusb_inference_init() {
    usb = make_shared<usb_io::Peer>();
}

static vector<unsigned char> get_model_name(InferenceModel model) {
    vector<unsigned char> model_name;
    switch (model) {
    case YOLOV2:
        model_name = vector<unsigned char>{'y', 'o', 'l', 'o', 'v', '2'};
        break;
    case YOLOV3:
        model_name = vector<unsigned char>{'y', 'o', 'l', 'o', 'v', '3'};
        break;
    case OPENPOSE:
        model_name = vector<unsigned char>{'o', 'p', 'e', 'n', 'p', 'o', 's', 'e'};
        break;
    default:
        throw invalid_argument("");
    }
    return model_name;
}

void libusb_inference_load_model(InferenceModel model) {
    vector<unsigned char> command{'l', 'o', 'a', 'd'};
    usb->put({
        memory::Bytes(move(command)),
        memory::Bytes(get_model_name(model))
    });
}

void libusb_inference_send(InferenceModel model, std::vector<unsigned char> &&input_jpg) {
    generic_image_t::metadata_t metadata;
    metadata.format = generic_image_t::JPG;
    auto packet = memory::Encoding<generic_image_t>::encode(generic_image_t(metadata, move(input_jpg)));
    packet.push_front(memory::Bytes(get_model_name(model)));
    usb->put(move(packet));
}

int libusb_inference_receive(std::vector<unsigned char> &output_jpg) {
    auto data = usb->get();
    if (!data.has_value()) {
        return -1;
    }

    LOGD << data.value();

    generic_image_t image;
    memory::Encoding<generic_image_t>::decode(data.value(), image);
    output_jpg.resize(image.data.size());
    memcpy(output_jpg.data(), image.data.data(), image.data.size());

    return 0;
}

int libusb_inference_receive_object_detect(std::vector<unsigned char> &output_jpg, std::vector<object_detect_rect_t> &result) {
    auto data = usb->get();
    if (!data.has_value()) {
        return -1;
    }

    generic_image_t image;
    memory::Encoding<generic_image_t>::decode(data.value(), image);
    memory::Encoding<vector<object_detect_rect_t>>::decode(data.value(), result);
    output_jpg.resize(image.data.size());
    memcpy(output_jpg.data(), image.data.data(), image.data.size());

    return 0;
}

void libusb_inference_free() {
    usb->close();
}
