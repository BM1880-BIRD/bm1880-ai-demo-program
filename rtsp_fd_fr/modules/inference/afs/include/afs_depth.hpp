#pragma once

#include "afs_base.hpp"
#include <face/anti_facespoofing/afs_depth.hpp>

namespace inference {
namespace pipeline {

class AFSDepth : public ::inference::pipeline::AFSBase {
public:
    explicit AFSDepth(const json &config);

protected:
    result_item_t getItem(std::tuple<image_t &, VectorPack &> tuple) override;

private:
    std::unique_ptr<qnn::vision::AntiFaceSpoofingDepth> core;
    float threshold = 0;
};

}
}