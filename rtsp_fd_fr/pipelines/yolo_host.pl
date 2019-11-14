#include "object_detection_encode.hpp"
{
pipeline::SourceWrap<VideoCaptureSource>(0)
pipeline::Encode<image_t>()
pipeline::SinkWrap<usb_io::Peer>()
pipeline::SourceWrap<usb_io::Peer>()
pipeline::Decode<image_t, vector<qnn::vision::object_detect_rect_t>>()
pipeline::SinkWrap<ImshowSink>("1")
}
