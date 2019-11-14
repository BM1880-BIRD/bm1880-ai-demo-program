#include "file_io.hpp"
#include <unistd.h>
#include <poll.h>
#include <termios.h>
#include <cstring>
#include <algorithm>
#include <cassert>
#include <cerrno>

using namespace std;

void FileIO::unread(unsigned char *buf, size_t count) {
    size_t offset = read_buffer_.size();
    read_buffer_.resize(offset + count);
    memcpy(read_buffer_.data() + offset, buf, count);
}

int FileIO::read(unsigned char *buf, size_t count) {
    return read(buf, count, count);
}

int FileIO::read(unsigned char *buf, size_t at_least, size_t at_most) {
    size_t bytes_read = min<size_t>(read_buffer_.size(), at_most);
    memcpy(buf, read_buffer_.data(), bytes_read);
    read_buffer_.erase(read_buffer_.begin(), read_buffer_.begin() + bytes_read);

    if ((fd_ >= 0) && (bytes_read < at_most)) {
        do {
            assert(at_most >= bytes_read);
            int ret = ::read(fd_, buf + bytes_read, at_most - bytes_read);
            if (ret > 0) {
                bytes_read += ret;
            } else {
                if ((ret < 0) && (errno != EAGAIN)) {
                    break;
                } else {
                    if (bytes_read >= at_least) {
                        break;
                    }

                    struct pollfd subject;
                    subject.fd = fd_;
                    subject.events = POLLIN;
                    if (poll(&subject, 1, 1000) <= 0) {
                        fprintf(stderr, "read poll or error %lu %lu %lu\n", bytes_read, at_least, at_most);
                        break;
                    }
                }
            }
        } while ((fd_ >= 0) && (bytes_read < at_least));
    }

    return bytes_read;
}

bool FileIO::write(unsigned char *buf, size_t count) {
    size_t bytes_written = 0;

    flush_write();
    if (!write_buffer_.empty()) {
        write_buffer_.resize(write_buffer_.size() + count);
        memcpy(write_buffer_.data() + write_buffer_.size() - count, buf, count);
        return true;
    }

    while ((fd_ >= 0) && (bytes_written < count)) {
        int ret = ::write(fd_, buf + bytes_written, count - bytes_written);
        if (ret <= 0) {
            if ((ret == 0) || (errno == EAGAIN)) {
                append_write_buffer(buf + bytes_written, count - bytes_written);
                return true;
            } else {
                return false;
            }
        } else {
            bytes_written += ret;
        }
    }

    return bytes_written == count;
}

int FileIO::flush_write() {
    if (write_buffer_.empty()) {
        return 0;
    }

    vector<unsigned char> buffer;
    write_buffer_.swap(buffer);
    return write(buffer.data(), buffer.size());
}

bool FileIO::buffered_write_data() const {
    return !write_buffer_.empty();
}

bool FileIO::buffered_read_data() const {
    return !read_buffer_.empty();
}

void FileIO::append_write_buffer(unsigned char *buf, size_t count) {
    write_buffer_.resize(write_buffer_.size() + count);
    memcpy(write_buffer_.data() + write_buffer_.size() - count, buf, count);
}