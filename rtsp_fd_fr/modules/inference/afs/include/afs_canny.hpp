#pragma once

#include "afs_base.hpp"
#include <face/anti_facespoofing/afs_canny.hpp>

namespace inference {
namespace pipeline {

class AFSCanny : public ::inference::pipeline::AFSBase {
public:
    explicit AFSCanny(const json &config);

protected:
    result_item_t getItem(std::tuple<image_t &, VectorPack &> tuple) override;

private:
    std::unique_ptr<qnn::vision::AntiFaceSpoofingCanny> core;
    float weighting = 0;
    float threshold = 0;
};

}
}