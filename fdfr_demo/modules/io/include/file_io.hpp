#pragma once

#include <unistd.h>
#include <memory>
#include <sys/file.h>
#include <vector>

class FileIO {
public:
    FileIO() : locked_(false), fd_(-1) {}
    FileIO(int fd) : locked_(false), fd_(fd) {}
    FileIO(const FileIO &) = delete;
    ~FileIO() {
        close();
    }

    FileIO &operator=(const FileIO &) = delete;
    FileIO &operator=(FileIO &&rhs) {
        close();

        fd_ = rhs.fd_;
        locked_ = rhs.locked_;

        rhs.fd_ = -1;
        rhs.locked_ = false;
        return *this;
    }

    void close() {
        if (fd_ >= 0) {
            if (locked_) {
                flock(fd_, LOCK_UN);
            }

            ::close(fd_);
        }

        fd_ = -1;
        locked_ = false;
    }

    void set_fd(int fd) {
        close();
        fd_ = fd;
        locked_ = false;
    }

    bool lock() {
        if ((fd_ >= 0) && (!locked_)) {
            locked_ = (flock(fd_, LOCK_EX | LOCK_NB) == 0);
        }

        return locked_;
    }

    int fd() const {
        return fd_;
    }

    void unread(unsigned char *buf, size_t count);
    int read(unsigned char *buf, size_t count);
    int read(unsigned char *buf, size_t at_least, size_t at_most);
    bool write(unsigned char *buf, size_t count);
    int flush_write();
    bool buffered_write_data() const;
    bool buffered_read_data() const;

private:
    void append_write_buffer(unsigned char *buf, size_t count);

    std::vector<unsigned char> read_buffer_;
    std::vector<unsigned char> write_buffer_;
    bool locked_;
    int fd_;
};
