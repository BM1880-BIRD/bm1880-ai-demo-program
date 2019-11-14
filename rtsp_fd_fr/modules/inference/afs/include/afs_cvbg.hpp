#pragma once

#include "afs_base.hpp"
#include <face/anti_facespoofing/afs_cv.hpp>

namespace inference {
namespace pipeline {

class AFSCVBG : public ::inference::pipeline::AFSBase {
public:
    explicit AFSCVBG(const json &config);

protected:
    result_item_t getItem(std::tuple<image_t &, VectorPack &> tuple) override;

private:
    std::unique_ptr<qnn::vision::AntiFaceSpoofingCVBG> core;
};

}
}