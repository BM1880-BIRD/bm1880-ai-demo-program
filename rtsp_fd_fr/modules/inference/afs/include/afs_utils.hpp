#pragma once

#include "vector_pack.hpp"
#include "json.hpp"
#include <vector>
#include <string>
#include <iostream>

namespace inference {
namespace pipeline {

struct result_item_t {
    std::string type;
    bool liveness = false;
    float confidence = -1;
    float threshold = -1;
    float weighting = -1;
};

struct result_items_t {
    std::vector<result_item_t> data;
};

inline void to_json(json &j, const result_items_t &t) {
    j = json::array();

    for (auto &item: t.data) {
        json obj = json{{"type", item.type},
                        {"liveness", item.liveness},
                        {"confidence", item.confidence},
                        {"threshold", item.threshold},
                        {"weighting", item.weighting}};

        j.push_back(obj);
    }
}
inline void from_json(const json &j, result_items_t &t) {
    for (auto &obj: j) {
        result_item_t item;

        item.type = obj.at("type").get<std::string>();
        item.liveness = j.at("liveness").get<bool>();
        item.confidence = j.at("confidence").get<float>();
        item.threshold = j.at("threshold").get<float>();
        item.weighting = j.at("weighting").get<float>();

        t.data.emplace_back(std::move(item));
    }
}

}
}

REGISTER_UNIQUE_TYPE_NAME(inference::pipeline::result_items_t);