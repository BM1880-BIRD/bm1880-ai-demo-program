#include <functional>
#include <memory>
#include "component_factory.hpp"
#include "string_parser.hpp"
#include "face_register_pipe.hpp"
#include "face_attr_pipe.hpp"
#include "pose_detection_pipe.hpp"
#include "http_request_sink.hpp"
#include "video_capture_source.hpp"
#include "image_file_source.hpp"
#include "pipe_vector_wrap.hpp"
#include "fdfr_pipe.hpp"
#include "pipeline_core.hpp"
#include "image_http_encode.hpp"
#include "object_detection_pipe.hpp"
#include "usb_comm.hpp"
#include "antifacespoofing.hpp"
#include "afs_classify.hpp"
#include "afs_classify_hsv_ycbcr.hpp"
#include "afs_canny.hpp"
#include "afs_patch.hpp"
#include "afs_lbpsvm.hpp"
#include "afs_cvbg.hpp"
#include "afs_depth.hpp"
#include "afs_cvsharp.hpp"
#include "afs_result.hpp"
#include "afs_led_controller.hpp"
#include "vector_pack_dump.hpp"
#include "zktcam_source.hpp"

using namespace std;

template <typename T>
inline shared_ptr<pipeline::TypeErasedComponent> create_component(const json &config) {
    return make_type_erased(make_shared<T>(config));
}

template <typename T>
inline function<T(const json &)> get_from_json(const string &key) {
    return [key](const json &j) {
        return j.at(key).get<T>();
    };
}

template <typename C, typename... T>
inline function<shared_ptr<pipeline::TypeErasedComponent>(const json &)> create_component_wrap(function<T(const json &)> &&...getters) {
    return [=](const json &j) {
        return pipeline::make_type_erased(make_shared<C>(getters(j)...));
    };
}

#define ARG(KEY, TYPE)  get_from_json<TYPE>(KEY)
#define CONSTRUCTOR(COMP, ...) create_component_wrap<COMP>(__VA_ARGS__)

namespace pipeline {
namespace preset {

ComponentFactory::ComponentFactory(const json &conf) : config(conf) {
    creators["face_detection"] = create_component<face::pipeline::FD>;
    creators["face_recognition"] = create_component<face::pipeline::FR>;
    creators["face_identification"] = create_component<face::pipeline::Identify>;
    creators["face_attribute_stabilizer"] = create_component<face::pipeline::FaceAttrStabilizer>;
    creators["pose_detection"] = create_component<pose::pipeline::PoseDetection>;
    creators["image_files"] = create_component<pipeline::SourceWrap<ImageFileSource>>;
    creators["video_capture"] = create_component<pipeline::SourceWrap<VideoCaptureSource>>;
    creators["antifacespoofing"] = create_component<inference::pipeline::AntiFaceSpoofing>;
    creators["afs_classify"] = create_component<inference::pipeline::AFSClassify>;
    creators["afs_classify_hsv_ycbcr"] = create_component<inference::pipeline::AFSClassifyHSVYCbCr>;
    creators["afs_canny"] = create_component<inference::pipeline::AFSCanny>;
    creators["afs_patch"] = create_component<inference::pipeline::AFSPatch>;
    creators["afs_lbpsvm"] = create_component<inference::pipeline::AFSLBPSVM>;
    creators["afs_cvbg"] = create_component<inference::pipeline::AFSCVBG>;
    creators["afs_depth"] = create_component<inference::pipeline::AFSDepth>;
    creators["afs_cvsharp"] = create_component<inference::pipeline::AFSCVSharpness>;
    creators["afs_result"] = create_component<inference::pipeline::AFSResult>;
    creators["afs_led_controller"] = create_component<inference::pipeline::LEDController>;
    creators["vectorpackdump"] = create_component<pipeline::SinkWrap<VectorPackDump>>;
    creators["ztkcam_source"] = create_component<pipeline::SourceWrap<ZktCameraSource>>;
    creators["multipart<image_t, VectorPack>"] = create_component<pipeline::SinkWrap<HttpMultipartSink<image_t, VectorPack>>>;
    creators["object_detection"] = create_component<object_detection::pipeline::ObjectDetection>;
    creators["register_face"] = create_component<face::pipeline::Register>;
}

shared_ptr<TypeErasedComponent> ComponentFactory::create(const string &name) {
    try {
        auto iter = creators.find(config.at(name).at("type").get<string>());
        if (iter == creators.end()) {
            throw runtime_error("component type [" + config.at(name).at("type").get<string>() + "] is undefined");
        }
        return iter->second(config.at(name));
    } catch (nlohmann::detail::exception &e) {
        fprintf(stderr, "%s:%d json exception in constructing component [%s]\n", __FILE__, __LINE__, name.data());
        throw e;
    }
}

}
}
