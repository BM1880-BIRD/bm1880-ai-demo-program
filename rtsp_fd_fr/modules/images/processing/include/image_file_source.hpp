#pragma once

#include <string>
#include <queue>
#include <vector>
#include <deque>
#include <set>
#include <dirent.h>
#include "data_source.hpp"
#include "image.hpp"
#include "json.hpp"

class ImageFileSource : public DataSource<std::string, image_t> {
public:
    explicit ImageFileSource(const json &config);
    explicit ImageFileSource(const std::string &path, const std::vector<std::string> &extensions, bool recursive = false);
    ~ImageFileSource();

    mp::optional<std::string, image_t> get() override;
    bool is_opened() override;
    void close() override;

private:
    void load_dir();

    bool recur;
    std::set<std::string> exts;

    std::queue<std::string> dirs;
    std::deque<std::string> files;
};