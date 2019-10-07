#pragma once

#include <vector>
#include <array>
#include <string>
#include <type_traits>
#include <fstream>
#include <map>
#include <list>
#include <cstring>
#include "../../utils/include/file_utils.hpp"
#include "path.hpp"
#include "db_types.hpp"
#include "db_field.hpp"
#include "optional.hpp"
#include "path.hpp"

namespace database {

template <typename... Fields>
class Database {
public:
    using entry_t = Entry<Fields...>;
    static constexpr size_t entry_per_page = 10;

    Database(const std::string &path) {
        if (!::utils::file::exists(path)) {
            auto parent = path.substr(0, path.find_last_of('/'));
            mkpath(parent, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            std::ofstream fout(path);
        } else if (!::utils::file::is_regular(path)) {
            throw std::runtime_error("db file [" + path + "] is not a regular file.");
        }

        load(path);
    }

    void load(const std::string &path) {
        path_ = path;
        file_.open(path_, std::ios_base::binary | std::ios_base::in | std::ios_base::out);
        file_.seekg(0);

        if (!file_.is_open()) {
            throw std::runtime_error("cannot open file [" + path_ + "].");
        }

        read_data_pages();
    }

    mp::optional<size_t> add(entry_t &&fields) {
        uint32_t id = (data_.empty() ? 1 : data_.rbegin()->first + 1);

        set(id, std::move(fields));

        return mp::optional<size_t>(mp::inplace_t(), id);
    }

    void set(size_t id, entry_t &&fields) {
        auto iter = data_.find(id);
        if (iter != data_.end()) {
            auto &entry = iter->second;

            for (size_t i = 0; i < sizeof...(Fields); i++) {
                fields.set_file_pos(i, entry.file_pos(i));
            }

            entry = std::move(fields);

            for (size_t i = 0; i < sizeof...(Fields); i++) {
                auto pos = entry.file_pos(i);
                if (pos.has_value()) {
                    file_.seekp(pos.value());
                    entry.write(file_, i);
                }
            }
        } else {
            for (size_t field_id = 0; field_id < sizeof...(Fields); field_id++) {
                if (avail_field_locations_[field_id].empty()) {
                    append_page(field_id);
                }

                auto offset = avail_field_locations_[field_id].front();
                avail_field_locations_[field_id].pop_front();

                file_.seekp(offset);
                file_.write(reinterpret_cast<char*>(&id), sizeof(uint32_t));
                fields.write(file_, field_id);

                file_.flush();
            }

            data_.emplace(id, std::move(fields));
        }
    }

    bool remove(size_t id) {
        auto iter = data_.find(id);
        if (iter != data_.end()) {
            entry_t &entry = iter->second;

            for (size_t i = 0; i < sizeof...(Fields); i++) {
                auto offset = entry.file_pos(i);
                if (offset.has_value()) {
                    uint32_t zero = 0;
                    file_.seekp(offset.value() - sizeof(uint32_t));
                    file_.write(reinterpret_cast<char*>(&zero), sizeof(uint32_t));
                    avail_field_locations_[i].emplace_back(offset.value() - sizeof(uint32_t));
                }
            }

            data_.erase(iter);
        }
        return true;
    }

    void remove_all() {
        data_.clear();
        avail_field_locations_.clear();
        file_.close();
        file_.open(path_, std::ios_base::in | std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
    }

    size_t size() const {
        return data_.size();
    }

    std::vector<size_t> id_list() const {
        std::vector<size_t> ids;
        ids.reserve(data_.size());

        for (auto &p : data_) {
            ids.emplace_back(p.first);
        }

        return ids;
    }

    mp::optional<entry_t> get(size_t id) const {
        mp::optional<entry_t> data;
        auto iter = data_.find(id);

        if (iter != data_.end()) {
            data.emplace(iter->second);
        }

        return data;
    }

    template <typename Fn>
    void for_each(Fn &&f) {
        for (auto &p : data_) {
            if (!f(p.first, p.second)) {
                break;
            }
        }
    }

private:
    void read_data_pages() {
        while (true) {
            try {
                read_data_page();
            } catch (std::ios_base::failure &e) {
                break;
            }
        }
    }

    void read_data_page() {
        data_page_t page;
        file_.read(reinterpret_cast<char*>(&page), sizeof(page));

        if (file_.eof() || (size_t)file_.gcount() < sizeof(page)) {
            file_.clear();
            throw std::ios_base::failure("eof");
        }

        if (strncmp(page.magic_number, data_page_t::page_magic(), sizeof(page.magic_number)) != 0) {
            throw std::runtime_error("data page in db file [" + path_ + "] is invalid");
        } else if (page.field_id >= sizeof...(Fields)) {
            throw std::runtime_error("data page in db file [" + path_ + "] is invalid");
        } else if ((static_cast<data_type>(page.field_data_type) != entry_t::type_of(page.field_id)) ||
                   (page.field_data_length != entry_t::length_of(page.field_id))) {
            throw std::runtime_error("data page in db file [" + path_ + "] is invalid");
        }

        size_t entry_length = sizeof(uint32_t) + entry_t::bytecount_of(page.field_id);

        for (size_t i = 0; i < page.entry_count; i++) {
            uint32_t id;
            file_.read(reinterpret_cast<char*>(&id), sizeof(uint32_t));

            if (id == 0) {
                auto offset = file_.tellg();
                avail_field_locations_[page.field_id].emplace_back(offset - sizeof(uint32_t));
                file_.seekg(offset - sizeof(uint32_t) + entry_length);
            } else {
                data_[id].read(file_, page.field_id);
            }
        }
    }

    void append_page(size_t field_id) {
        file_.seekp(0, std::ios_base::end);

        data_page_t page;
        strncpy(page.magic_number, data_page_t::page_magic(), sizeof(page.magic_number));
        page.field_id = field_id;
        page.field_data_type = static_cast<uint32_t>(entry_t::type_of(field_id));
        page.field_data_length = static_cast<uint32_t>(entry_t::length_of(field_id));
        page.entry_count = entry_per_page;
        file_.write(reinterpret_cast<char*>(&page), sizeof(data_page_t));

        std::vector<char> zeroes(sizeof(uint32_t) + entry_t::bytecount_of(field_id), 0);
        for (size_t i = 0; i < entry_per_page; i++) {
            avail_field_locations_[field_id].emplace_back(file_.tellp());
            file_.write(zeroes.data(), zeroes.size());
        }

        file_.flush();
    }

    std::fstream file_;
    std::string path_;
    std::map<size_t, entry_t> data_;
    std::map<size_t, std::list<size_t>> avail_field_locations_;
};

}
