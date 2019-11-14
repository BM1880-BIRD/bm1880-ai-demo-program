#pragma once

#include "afs_base.hpp"

namespace inference {
namespace pipeline {

class LEDController : public ::inference::pipeline::AFSBase {
public:
    explicit LEDController(const json &config);
    std::tuple<> process(std::tuple<image_t &, VectorPack &> tuple);

protected:
    result_item_t getItem(std::tuple<image_t &, VectorPack &> tuple) override;

private:
    float mean_threshold = 0;
    float std_dev_threshold = 0;
};

}
}