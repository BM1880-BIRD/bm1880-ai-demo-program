#include "yolo.hpp"
#include <memory>
#include <sstream>
#include "usb_comm.hpp"
#include "memory/bytes.hpp"
#include "memory/encoding.hpp"
#include "generic_image.hpp"

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

void remote_yolo_init() {
    usb = make_shared<usb_io::Peer>();
}

void remote_yolo_send(vector<unsigned char> &&input_jpg) {
    generic_image_t::metadata_t metadata;
    metadata.format = generic_image_t::JPG;

    auto packet = memory::Encoding<generic_image_t>::encode(generic_image_t(metadata, memory::Bytes(move(input_jpg))));
    usb->put(move(packet));
}

int remote_yolo_receive(vector<unsigned char> &output_jpg, vector<object_detect_rect_t> &result) {
    auto data = usb->get();
    if (!data.has_value()) {
        return -1;
    }

    generic_image_t img;
    memory::Encoding<generic_image_t>::decode(data.value(), img);
    memory::Encoding<vector<object_detect_rect_t>>::decode(data.value(), result);

    output_jpg.resize(img.data.size());
    memcpy(output_jpg.data(), img.data.data(), img.data.size());

    return 0;
}

void remote_yolo_free() {
    usb->close();
}
