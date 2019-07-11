#include "file_utils.hpp"
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <stdio.h>

namespace utils {
namespace file {

bool exists(const std::string &path) {
    struct stat statbuf;

    return 0 == stat(path.data(), &statbuf);
}

bool is_regular(const std::string &path) {
    struct stat statbuf;
    if (0 != stat(path.data(), &statbuf)) {
        return false;
    }

    return (statbuf.st_mode & S_IFMT) == S_IFREG;
}

bool is_directory(const std::string &path) {
    struct stat statbuf;
    if (0 != stat(path.data(), &statbuf)) {
        return false;
    }

    return (statbuf.st_mode & S_IFMT) == S_IFDIR;
}

std::string content(const std::string &path, size_t offset, size_t length) {
    std::vector<char> buf(length);
    FILE *fp = fopen(path.data(), "r");

    fseek(fp, offset, SEEK_SET);
    length = fread(buf.data(), 1, length, fp);
    fclose(fp);

    return std::string(buf.data(), length);
}

bool remove(const std::string &path) {
    return (0 == ::remove(path.data()));
}

}
}