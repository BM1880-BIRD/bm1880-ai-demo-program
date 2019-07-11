#pragma once

#include "annoylib.h"
#include "kissrandom.h"
#include "log_common.h"
#include "repo.hpp"
#include "path.hpp"
#include <fstream>
#include <stdexcept>
#include <cassert>

#define TOPK 1
#define TREE_NUM 1
#define NAMES_REPO__FILENAME "/names.bin"
#define ANNOY_REPO__FILENAME "/annoy.tree"

template <typename T, size_t DIM>
class AnnoyRepo final : public Repo<DIM> {
    public:
    typedef typename Repo<DIM>::feature_t feature_t;

    explicit AnnoyRepo(const std::string &path)
        : path_(path)
        , annoy_(new AnnoyIndex<int, T, Angular, Kiss32Random>(DIM)) {
        mkpath(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        load();
    };

    AnnoyRepo(AnnoyRepo &&rhs)
        : repo_size(rhs.repo_size)
        , cur_size(rhs.cur_size)
        , path_(std::move(rhs.path_))
        , names_(std::move(rhs.names_))
        , annoy_(std::move(rhs.annoy_)) {};

    virtual ~AnnoyRepo() {}

    virtual size_t add(const std::string &name,
    const feature_t &features) override {
        size_t id = cur_size;
        assert(features.size() == size_t(DIM));
        annoy_->add_item(int(id), features.data());
        assert(names_.size() == cur_size);
        names_.emplace_back(name);
        ++cur_size;

        return id;
    }

    void build() override {
        if (cur_size == repo_size) {
            return; // nothing changed
        }

        repo_size = cur_size;

        annoy_->build(TREE_NUM);
        annoy_->save((path_ + ANNOY_REPO__FILENAME).c_str());

        std::ofstream name_file((path_ + NAMES_REPO__FILENAME), std::ofstream::out);
        name_file << std::to_string(names_.size()) << std::endl;
        for (auto n : names_) {
            name_file << n << std::endl;
        }
    };

    size_t size() const override {
        return cur_size;
    }

    std::vector<size_t> id_list() const override {
        std::vector<size_t> ids;
        ids.reserve(cur_size);
        for (size_t i = 0; i < cur_size; i++) {
            ids[i] = i;
        }

        return ids;
    }

    mp::optional<std::pair<std::string, feature_t>> get(size_t id) const override {
        mp::optional<std::pair<std::string, feature_t>> result;
        if (id < cur_size) {
            std::vector<T> vec(DIM);
            annoy_->get_item(id, vec.data());
            result.emplace(names_[id], std::move(vec));
        }
        return result;
    }

    mp::optional<std::string> get_name(size_t id) const override {
        mp::optional<std::string> result;
        if (id < cur_size) {
            result.emplace(names_[id]);
        }
        return result;
    }

    typename Repo<DIM>::match_result match(const feature_t &features, double threshold) override {
        using result_t = typename Repo<DIM>::match_result;
        std::vector<int> toplist;
        std::vector<T> distances;
        annoy_->get_nns_by_vector(features.data(), (size_t)TOPK, (size_t)-1, &toplist, &distances);
        assert(toplist.size() == TOPK);
        assert(distances.size() == toplist.size());
        assert(size_t(toplist[0]) < names_.size());
        T dist = 1.0 - distances[0];
        return result_t{dist > threshold, static_cast<size_t>(toplist[0]), dist};
    }

    private:
    void load() {
        if (!annoy_->load((path_ + ANNOY_REPO__FILENAME).c_str())) {
            LOGD << "can't find annoy repo file" << std::endl;
            unload();
            return;
        }

        std::ifstream name_file((path_ + NAMES_REPO__FILENAME));

        std::string str;
        if (!std::getline(name_file, str)) {
            LOGD << "can't find names repo file" << std::endl;
            unload();
            return;
        }

        try {
            std::string::size_type sz;
            repo_size = std::stoi(str, &sz);
        } catch (...) {
            LOGD << "parse length in names repo failed" << std::endl;
            unload();
            return;
        }

        size_t count = 0;
        while (std::getline(name_file, str)) {
            names_.emplace_back(str);
            ++count;
        }

        if (count != repo_size) {
            LOGD << "inconsist names repo" << std::endl;
            unload();
            return;
        }

        if (size_t(annoy_->get_n_items()) != repo_size) {
            LOGD << "inconsist names & annoy repo" << std::endl;
            unload();
            return;
        }
    }

    void unload() {
        annoy_->unload();
        names_.clear();
        repo_size = 0;
        cur_size = 0;
    }

    size_t repo_size = 0;
    size_t cur_size = 0;
    std::string path_;
    std::vector<std::string> names_;
    std::unique_ptr<AnnoyIndex<int, T, Angular, Kiss32Random>> annoy_;
};
