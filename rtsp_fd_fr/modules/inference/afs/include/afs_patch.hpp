#pragma once

#include "afs_base.hpp"
#include <face/anti_facespoofing/afs_patch.hpp>

namespace inference {
namespace pipeline {

class AFSPatch : public ::inference::pipeline::AFSBase {
public:
    explicit AFSPatch(const json &config);

protected:
    result_item_t getItem(std::tuple<image_t &, VectorPack &> tuple) override;

private:
    std::unique_ptr<qnn::vision::AntiFaceSpoofingPatch> core;
};

}
}