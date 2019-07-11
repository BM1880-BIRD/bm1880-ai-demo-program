#pragma once

#include "repo.hpp"
#include <cmath>
#include <dirent.h>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <utility>
#include <vector>
#include <memory>
#include "path.hpp"
#include "file_utils.hpp"
#include "database.hpp"

template <size_t DIM>
class DBRepo : public MutableRepo<DIM> {
public:
    using typename MutableRepo<DIM>::feature_t;
    static constexpr size_t dim = DIM;

protected:
    static constexpr size_t name_length = 256;
    using database_t = database::Database<
        database::Field<database::data_type::char8, name_length>,
        database::Field<database::data_type::float32, DIM>
    >;
    using entry_t = typename database_t::entry_t;
    std::unique_ptr<database_t> db_;

public:
    explicit DBRepo(const std::string &path) : db_(new database_t(path)) {
    }

    DBRepo(DBRepo &&rhs) : db_(std::move(rhs.db_)) {}

    virtual ~DBRepo() {}

    size_t add(const std::string &name, const feature_t &features) override {
        entry_t entry;
        strncpy(database::get<0>(entry).data(), name.data(), name_length);
        database::get<1>(entry) = features;

        auto id = db_->add(std::move(entry));
        if (id.has_value()) {
            return id.value();
        } else {
            throw std::runtime_error("failed adding entry");
        }
    }

    void build() override {};

    typename Repo<DIM>::match_result match(const feature_t &features, double threshold) override {
        typename Repo<DIM>::match_result best{false, 0, 0};

        db_->for_each([this, &best, &features, &threshold](size_t id, entry_t &entry) {
            double score = compute_score(database::get<1>(entry), features);
            if (score > best.score) {
                best.score = score;
                best.id = id;
                if (score >= threshold) {
                    best.matched = true;
                }
            }
            return true;
        });

        return best;
    }

    double compute(size_t id, const feature_t &features) override {
        mp::optional<entry_t> entry = db_->get(id);
        if (!entry.has_value()) {
            return 0;
        }

        return compute_score(database::get<1>(entry.value()), features);
    }

    size_t size() const override {
        return db_->size();
    }

    std::vector<size_t> id_list() const override {
        return db_->id_list();
    }

    void set(size_t id, const std::string &name, const feature_t &features) override {
        entry_t entry;
        strncpy(database::get<0>(entry).data(), name.data(), name_length);
        database::get<1>(entry) = features;

        db_->set(id, std::move(entry));
    }

    mp::optional<std::pair<std::string, feature_t>> get(size_t id) const override {
        mp::optional<entry_t> entry = db_->get(id);
        mp::optional<std::pair<std::string, feature_t>> result;
        if (entry.has_value()) {
            std::string name = database::get<0>(entry.value()).data();
            feature_t feature = std::move(database::get<1>(entry.value()));

            result.emplace(std::move(name), std::move(feature));
        }

        return result;
    }

    mp::optional<std::string> get_name(size_t id) const override {
        mp::optional<entry_t> entry = db_->get(id);
        mp::optional<std::string> result;
        if (entry.has_value()) {
            std::string name = database::get<0>(entry.value()).data();

            result.emplace(std::move(name));
        }

        return result;
    }

    void remove(size_t id) override {
        db_->remove(id);
    }

    void remove_all() override {
        db_->remove_all();
    }

private:
    double compute_score(const feature_t &lhs, const feature_t &rhs) const {
        double lhs_sq_mag = 0;
        double rhs_sq_mag = 0;
        double cdot = 0;
        for (size_t i = 0; i < std::min<size_t>(lhs.size(), rhs.size()); i++) {
            cdot += lhs[i] * rhs[i];
            lhs_sq_mag += lhs[i] * lhs[i];
            rhs_sq_mag += rhs[i] * rhs[i];
        }
        if (lhs_sq_mag == 0 || rhs_sq_mag == 0) {
            return 0;
        }

        return cdot / sqrt(lhs_sq_mag) / sqrt(rhs_sq_mag);
    }
};
