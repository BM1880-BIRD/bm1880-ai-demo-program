
#pragma once

#include "afs_base.hpp"
#include <face/anti_facespoofing/afs_classify.hpp>

namespace inference {
namespace pipeline {

class AFSClassify : public ::inference::pipeline::AFSBase {
public:
    explicit AFSClassify(const json &config);

protected:
    result_item_t getItem(std::tuple<image_t &, VectorPack &> tuple) override;

private:
    std::unique_ptr<qnn::vision::AntiFaceSpoofingClassify> core;
    float weighting = 0;
    float threshold = 0;
    float scale_ratio = 0.6;
};

}
}