#include "afs_lbpsvm.hpp"
#include "face_api.hpp"

using namespace std;

namespace inference {
namespace pipeline {

AFSLBPSVM::AFSLBPSVM(const json &config) : AFSBase(config) {
    weighting = config.at("weighting").get<float>();
    threshold = config.at("threshold").get<float>();

    auto base_path = config.at("model_dir").get<string>();
    auto model_path = base_path + config.at("svm").get<string>();
    auto mean_path = base_path + config.at("mean").get<string>();
    auto std_path = base_path + config.at("std").get<string>();

    core.reset(new qnn::vision::AntiFaceSpoofingLBPSVM(model_path, mean_path, std_path));
    core->SetThreshold(threshold);
}

result_item_t AFSLBPSVM::getItem(std::tuple<image_t &, VectorPack &> tuple) {
    result_item_t item;
    auto &vp = get<1>(tuple);

    if (!vp.get<face_info_t>().size()) {
        return item;
    }

    bool is_real = core->Detect(get<0>(tuple), vp.get<face_info_t>()[0]);

    item.type = "lbpsvm";
    item.liveness = is_real;
    item.confidence = core->GetLbpSVMResult();
    item.threshold = threshold;
    item.weighting = weighting;

    return item;
}

}
}
