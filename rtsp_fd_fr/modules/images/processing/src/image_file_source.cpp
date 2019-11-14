#include "image_file_source.hpp"
#include <dirent.h>
#include <cstring>
#include "logging.hpp"

using namespace std;

ImageFileSource::ImageFileSource(const json &conf) :
ImageFileSource(conf.at("path").get<string>(), conf.at("extensions").get<vector<string>>(), conf.at("recursive").get<bool>()) {}

ImageFileSource::ImageFileSource(const string &path, const vector<string> &extensions, bool recursive) :
DataSource<std::string, image_t>("ImageFileSource"),
recur(recursive), exts(extensions.begin(), extensions.end()) {
    dirs.emplace(path);
}

ImageFileSource::~ImageFileSource() {
}

mp::optional<std::string, image_t> ImageFileSource::get() {
    while (!dirs.empty() && files.empty()) {
        load_dir();
    }
    if (files.empty()) {
        return mp::nullopt_t();
    }
    auto path = move(files.front());
    files.pop_front();

    try {
        LOGD << "reading image from file: " << path;
        auto img = cv::imread(path);
        auto offset = path.rfind('/') + 1;
        auto name = path.substr(offset, path.rfind('.') - offset);
        return mp::optional<std::string, image_t>(mp::inplace_t(), move(name), move(img));
    } catch (exception &e) {
        LOGI << "failed reading image from file: " << path;
        return get();
    }
}

bool ImageFileSource::is_opened() {
    return !dirs.empty() || !files.empty();
}

void ImageFileSource::close() {
    dirs = queue<string>();
    files.clear();
}

void ImageFileSource::load_dir() {
    auto path = dirs.front();
    dirs.pop();

    auto dir = opendir(path.data());
    if (dir == nullptr) {
        return;
    }

    dirent *ent;
    vector<string> file_entries;
    vector<string> dir_entries;

    while ((ent = readdir(dir)) != nullptr) {
        if (ent->d_type == DT_DIR) {
            if (recur && strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
                dir_entries.emplace_back(path + "/" + ent->d_name);
            }
        } else {
            char *dot = strrchr(ent->d_name, '.');
            if (dot != nullptr) {
                string ext(dot + 1);
                if (exts.count(ext) != 0) {
                    file_entries.emplace_back(path + "/" + ent->d_name);
                }
            }
        }
    }

    closedir(dir);

    sort(file_entries.begin(), file_entries.end());
    sort(dir_entries.begin(), dir_entries.end());

    for (auto &dir_name : dir_entries) {
        dirs.emplace(move(dir_name));
    }
    for (auto &file_name : file_entries) {
        files.emplace_back(move(file_name));
    }
}