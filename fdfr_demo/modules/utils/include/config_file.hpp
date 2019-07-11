#pragma once

#include <string>
#include <map>
#include "string_parser.hpp"

static const std::string DEFAULT_SECTION = "";

class ConfigFile {
public:
    ConfigFile();
    ConfigFile(const std::string &path);

    void setConfigPath(const std::string &path);
    void reload();
    void print() const;

    template <typename T>
    T get(const std::string &key) const {
        return get<T>(DEFAULT_SECTION, key);
    }

    template <typename T>
    T get(const std::string &section, const std::string &key) const {
        auto section_iter = section_data_.find(section);
        if (section_iter != section_data_.end()) {
            auto &data = section_iter->second;
            auto data_iter = data.find(key);
            if (data_iter != data.end()) {
                return parse_string<T>(data_iter->second);
            }
        }

        return parse_string<T>("");
    }

    template <typename T>
    T require(const std::string &key) const {
        return require<T>(DEFAULT_SECTION, key);
    }

    template <typename T>
    T require(const std::string &section, const std::string &key) const {
        auto section_iter = section_data_.find(section);
        if (section_iter != section_data_.end()) {
            auto &data = section_iter->second;
            auto data_iter = data.find(key);
            if (data_iter != data.end()) {
                return parse_string<T>(data_iter->second);
            }
        }

        throw std::runtime_error((section.empty() ? std::string() : "section: \"" + section + "\", ") + "key: \"" + key + "\" is required but not defined in config file.");
    }

    std::vector<std::string> getSectionsName() {
        std::vector<std::string> sections_name;
        for (auto &section_pair : section_data_) {
            sections_name.push_back(section_pair.first.data());
        }

        return sections_name;
    }

    std::map<std::string, std::string> getSection(const std::string &section) {
        return section_data_[section];
    }

private:
    std::string path_;
    std::map<std::string, std::map<std::string, std::string>> section_data_;
};