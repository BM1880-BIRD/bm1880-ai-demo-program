#include "config_file.hpp"
#include <fstream>
#include <cctype>
#include <cstdio>
#include <algorithm>

using namespace std;

static string trim(string &&s) {
    size_t last_nonspace = string::npos;
    size_t first_nonspace = string::npos;

    for (size_t i = 0; i < s.length(); i++) {
        if (!isspace(s[i])) {
            last_nonspace = i;
            if (first_nonspace == string::npos) {
                first_nonspace = i;
            }
        }
    }

    if (first_nonspace == string::npos) {
        return "";
    } else {
        return s.substr(first_nonspace, last_nonspace + 1 - first_nonspace);
    }
}

ConfigFile::ConfigFile() {
}

ConfigFile::ConfigFile(const string &path) : path_(path) {
    reload();
}

void ConfigFile::setConfigPath(const std::string &path) {
    path_ = path;
}

void ConfigFile::reload() {
    section_data_.clear();
    ifstream fin(path_);

    if (!fin) {
        return;
    }

    string section_name = DEFAULT_SECTION;
    while (!fin.eof()) {
        string line;
        getline(fin, line);
        size_t index;
        if (all_of(line.begin(), line.begin() + min(line.length(), line.find('#')), [](char c) { return isspace(c); })) {
            continue;
        }

        // Check section end
        size_t end_bracket_index = line.find('}');
        if (end_bracket_index != string::npos) {
            section_name = DEFAULT_SECTION;
            continue;
        }

        size_t semicolon_index = line.find(':');
        if (semicolon_index == string::npos) {
            continue;
        }

        auto key = trim(string(line.begin(), line.begin() + semicolon_index));
        auto value = trim(string(line.begin() + semicolon_index + 1, line.end()));

        // Check section start
        size_t start_bracket_index = value.find('{');
        if (start_bracket_index != string::npos) {
            section_name = key;
            continue;
        }

        if (!key.empty()) {
            section_data_[section_name][key] = value;
        }
    }
}

void ConfigFile::print() const {
    for (auto &section_pair : section_data_) {
        for (auto &pair : section_pair.second) {
            string section = section_pair.first.data();
            if (section.compare(DEFAULT_SECTION)) {
                fprintf(stderr, "[%s] ", section.data());
            }
            fprintf(stderr, "%s: %s\n", pair.first.data(), pair.second.data());
        }
    }
}
