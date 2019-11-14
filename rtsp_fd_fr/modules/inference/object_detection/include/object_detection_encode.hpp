#pragma once

#include "face/face_types.hpp"
#include "json.hpp"
#include "memory/encoding.hpp"
#include <list>
#include <sstream>

namespace qnn {
namespace vision {

void to_json(json &j, const object_detect_rect_t &info);
void from_json(const json &j, object_detect_rect_t &info);

}
}

namespace memory {
    template <>
    class Encoding<qnn::vision::object_detect_rect_t> {
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
        static std::list<Bytes> encode(qnn::vision::object_detect_rect_t &&data) {
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

        static void decode(std::list<Bytes> &list, qnn::vision::object_detect_rect_t &output) {
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