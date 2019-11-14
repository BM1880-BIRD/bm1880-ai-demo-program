#include "afs_result.hpp"
#include <string>
#include <opencv2/opencv.hpp>
#include "face_api.hpp"
#include "afs_utils.hpp"

using namespace std;

namespace inference {
namespace pipeline {

AFSResult::AFSResult(const json &config) {
    weighted_threshold = config.at("threshold").get<float>();
    vote_threshold = config.at("vote_threshold").get<float>();
    vote_iou_threshold = config.at("vote_iou_threshold").get<float>();
    vote_size = config.at("vote_size").get<float>();
}

std::tuple<> AFSResult::process(std::tuple<image_t &, VectorPack &> tuple) {
    auto &vp = get<1>(tuple);

    if (!vp.get<result_items_t>().size() || !vp.get<face_info_t>().size()) {
        return make_tuple();
    }

    auto &bbox = vp.get<face_info_t>()[0].bbox;
    cv::Rect face_rect(bbox.x1, bbox.y1, bbox.x2 - bbox.x1 + 1, bbox.y2 - bbox.y1 + 1);
    result_items_t items = vp.get<result_items_t>()[0];
    float weighted_confidence_sum = 0;
    float weighting_sum = 0;

    result_t result;
    result.liveness = true;

    for (auto &item: items.data) {
        weighted_confidence_sum += item.confidence * item.weighting;
        weighting_sum += item.weighting;

        result.liveness &= item.liveness;
    }

    result.weighted_confidence = weighted_confidence_sum / weighting_sum;
    result.weighted_threshold = weighted_threshold;
    result.weighted_liveness = (result.weighted_confidence > result.weighted_threshold);

    UpdateVoteQueue(face_rect, result.weighted_liveness);
    result.vote_liveness = GetVoteResult();

    vp.add(vector<result_t>{result});

    return make_tuple();
}

void AFSResult::UpdateVoteQueue(cv::Rect rect, bool liveness) {
    if (vote_queue.size()) {
        float intersect_area = (vote_rect & rect).area();
        float union_area = (vote_rect | rect).area();
        float iou_val = intersect_area / union_area;

        if (iou_val < vote_iou_threshold) {
            vote_queue.clear();
        }
    }

    vote_rect = rect;
    vote_queue.emplace_back(liveness);

    if (vote_queue.size() > vote_size) {
        vote_queue.pop_front();
    }
}

bool AFSResult::GetVoteResult() {
	if (vote_queue.size() < vote_size) {
		return false;
	}
    int is_real_cnt = std::count(vote_queue.begin(), vote_queue.end(), true);
    float real_ratio = float(is_real_cnt) / vote_queue.size();

    return (real_ratio > vote_threshold);
}

}
}