#include "antifacespoofing.hpp"
#include <string>
#include <opencv2/opencv.hpp>
#include "face_api.hpp"

using namespace std;

static bool is_enable_config(const json &config, const char *name) {
    return config.contains(name) && config.at(name).contains("enable") && config.at(name).at("enable").get<bool>();
}

namespace inference {
namespace pipeline {

AntiFaceSpoofing::AntiFaceSpoofing(const json &config) {
    auto model_dir = config.at("model_dir").get<string>();
    auto depth_model = model_dir + "/face_spoof_depth.bmodel";
    auto optflow_model = model_dir + "/face_spoof_optflow.bmodel";
    auto patch_model = model_dir + "/face_spoof_patch.bmodel";
    auto classify_model = model_dir + "/face_spoof_classify.bmodel";
    auto classify_hsv_ycbcr_model = model_dir + "/face_spoof_classify_hsv_ycbcr.bmodel";
    auto canny_model = model_dir + "/face_spoof_canny.bmodel";
    core.reset(new qnn::vision::AntiFaceSpoofing(depth_model, optflow_model,
                                                patch_model, classify_model,
                                                classify_hsv_ycbcr_model, canny_model));

    core->UseTrackerQueue(config.at("use_queue").get<bool>());
    core->UseWeightingResult(config.at("use_weighting").get<bool>(),
    config.at("weighting_threshold").get<float>());
    output_confidence = config.at("use_weighting").get<bool>();

    core->SetDoFilter(
        is_enable_config(config, "cvbg"),
        is_enable_config(config, "classify"),
        is_enable_config(config, "classify_hsv_ycbcr"),
        is_enable_config(config, "cvsharpness"),
        is_enable_config(config, "lbpsvm"),
        is_enable_config(config, "depth"),
        is_enable_config(config, "optflow"),
        is_enable_config(config, "patch"),
        is_enable_config(config, "canny")
    );

    if (is_enable_config(config, "classify")) {
        core->SetClassifyFilterThreshold(config.at("classify").at("threshold").get<float>());
        core->SetClassifyWeighting(config.at("classify").at("weight").get<float>());
    }
    if (is_enable_config(config, "classify_hsv_ycbcr")) {
        core->SetClassifyHSVYCbCrFilterThreshold(config.at("classify_hsv_ycbcr").at("threshold").get<float>());
        core->SetClassifyHSVYCbCrWeighting(config.at("classify_hsv_ycbcr").at("weight").get<float>());
    }
    if (is_enable_config(config, "cvsharpness")) {
        core->SetSharpnessFilterThreshold(config.at("cvsharpness").at("threshold").get<int>());
    }
    if (is_enable_config(config, "depth")) {
        core->SetDepthFilterThreshold(config.at("depth").at("threshold").get<int>());
    }
    if (is_enable_config(config, "canny")) {
        core->SetCannyFilterThreshold(config.at("canny").at("threshold").get<float>());
    }
    if (is_enable_config(config, "lbpsvm")) {
        auto svm_model = model_dir + config.at("model_files").at("svm").get<string>();
        auto mean_model = model_dir + config.at("model_files").at("mean").get<string>();
        auto std_model = model_dir + config.at("model_files").at("std").get<string>();
        core->LoadLbpSVMModel(svm_model, mean_model, std_model);
        core->SetLbpSVMFilterThreshold(config.at("lbpsvm").at("threshold").get<int>());
    }
}

std::tuple<> AntiFaceSpoofing::process(std::tuple<image_t &, VectorPack &> tuple) {
    auto &vp = get<1>(tuple);
    vector<bool> results;
    vector<cv::Rect> rects;

    if (vp.get<face_info_t>().size())
        core->SetFaceInfo(vp.get<face_info_t>()[0]);
    for (int i = 0; i < vp.get<face_info_t>().size(); i++) {
        auto &bbox = vp.get<face_info_t>()[i].bbox;
        float x = bbox.x1;
        float y = bbox.y1;
        float w = bbox.x2 - bbox.x1 + 1;
        float h = bbox.y2 - bbox.y1 + 1;
        rects.emplace_back(x, y, w, h);
    }
    if (!rects.empty()) {
        core->Detect(get<0>(tuple), rects, results);

        vector<result_t> output(results.size());
        for (int i = 0; i < output.size(); i++) {
            output[i].liveness = results[i];
            if (output_confidence) {
                output[i].classify_confidence = core->GetConfidenceVal();
                output[i].classifyhsv_confidence = core->GetClassifyHSVYCbCrConfidenceVal();
                output[i].wegithed_confidence = core->GetWeightedConfidenceVal();
            } else {
                output[i].wegithed_confidence = 1;
            }
        }
        for (int i = 0; i < output.size() && i < vp.get<result_t>().size(); i++) {
            output[i].liveness = output[i].liveness && vp.get<result_t>()[i].liveness;
            if (!output_confidence) {
                output[i].wegithed_confidence = vp.get<result_t>()[i].liveness;
            }
        }
        vp.add(move(output));
        return std::tuple<>();
    }
    return make_tuple();
}

}
}
