#include "zktcam_source.hpp"
#include "zkt.hpp"

ZktCameraSource::ZktCameraSource(const json &) :
DataSource<image_t>("ZktCameraSource")
{
    opened.store(ZktCameraFam600Init());
}

mp::optional<image_t> ZktCameraSource::get() {
    mp::optional<image_t> result;
    if (opened.load()) {
        cv::Mat frame;
        if (ZktFam600GetFrame(frame) == 0) {
            result.emplace(std::move(frame));
        }
    }
    return result;
}

bool ZktCameraSource::is_opened() {
    return opened.load();
}

void ZktCameraSource::close() {
    opened.store(false);
}
