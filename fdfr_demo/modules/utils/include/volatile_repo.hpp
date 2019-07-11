#pragma once

#include <utility>
#include <map>
#include <vector>
#include "repo.hpp"
#include "similarity.hpp"

template <size_t DIM>
class VolatileRepo : public MutableRepo<DIM> {
private:
    using feature_t = typename MutableRepo<DIM>::feature_t;
    using match_result = typename MutableRepo<DIM>::match_result;
    std::map<size_t, std::pair<std::string, feature_t>> faces_;
    size_t capacity;

public:
    size_t add(const std::string &name, const feature_t &features) override {
        while ((capacity > 0) && faces_.size() >= capacity) {
            faces_.erase(faces_.begin());
        }

        size_t id = faces_.empty() ? 1 : (faces_.rbegin()->first + 1);
        faces_[id] = {name, features};

        return id;
    }

    void build() override {}

    match_result match(const feature_t &features, double threshold) override {
        typename Repo<DIM>::match_result best{false, 0, 0};
        for (auto &pair : faces_) {
            double score = face::compute_score(pair.second.second.begin(), pair.second.second.end(), features.begin(), features.end());
            if (score > best.score) {
                best.score = score;
                best.id = pair.first;
                if (score >= threshold) {
                    best.matched = true;
                }
            }
        }

        return best;
    }

    size_t size() const override {
        return faces_.size();
    }

    std::vector<size_t> id_list() const override {
        std::vector<size_t> ids;
        ids.reserve(faces_.size());
        for (auto &p : faces_) {
            ids.emplace_back(p.first);
        }
        return ids;
    }

    mp::optional<std::pair<std::string, feature_t>> get(size_t id) const override {
        auto iter = faces_.find(id);
        mp::optional<std::pair<std::string, feature_t>> result;
        if (iter != faces_.end()) {
            result.emplace(iter->second);
        }
        return result;
    }

    mp::optional<std::string> get_name(size_t id) const override {
        auto iter = faces_.find(id);
        mp::optional<std::string> result;
        if (iter != faces_.end()) {
            result.emplace(iter->second.first);
        }
        return result;
    }

    double compute(size_t id, const feature_t &features) override {
        auto iter = faces_.find(id);
        if (iter == faces_.end()) {
            return 0;
        }
        return face::compute_score(iter->second.second.begin(), iter->second.second.end(), features.begin(), features.end());
    }

    void set(size_t id, const std::string &name, const feature_t &features) override {
        auto iter = faces_.find(id);
        if (iter != faces_.end()) {
            iter->second.first = name;
            iter->second.second = features;
        }
    }

    void remove(size_t id) override {
        faces_.erase(id);
    }

    void remove_all() override {
        faces_.clear();
    }
};
