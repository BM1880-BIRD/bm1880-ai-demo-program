#include "tty_io.hpp"
#include <algorithm>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <sys/file.h>
#include <poll.h>
#include <iostream>
#include <cassert>

using namespace std;

const char TtyHeader::header_magic[TtyHeader::MagicLength::value] = "BMTXHDR";

void set_magic(TtyHeader &header) {
    memcpy(header.magic, TtyHeader::header_magic, TtyHeader::MagicLength::value);
}

constexpr size_t MaxPacketLength = 8192;
constexpr size_t PacketPadidngLength = 0;
constexpr size_t MaxDataLength = MaxPacketLength - PacketPadidngLength - sizeof(TtyHeader);

struct termios TtyConfig::settings() const {
    struct termios new_settings;

    memset(&new_settings, 0, sizeof(new_settings));

    new_settings.c_cflag |= (CLOCAL | CREAD | CS8);
    new_settings.c_lflag |= ~(ICANON | ECHO | ECHOE | ISIG);
    new_settings.c_oflag |= ~OPOST;
    new_settings.c_cc[VMIN] = 0;
    new_settings.c_cc[VTIME] = 0;

    cfsetispeed(&new_settings, baud);
    cfsetospeed(&new_settings, baud);

    return new_settings;
}

static void print_cflags() {
    fprintf(stderr, "CBAUD: %x\n", CBAUD);
    fprintf(stderr, "CBAUDEX: %x\n", CBAUDEX);
    fprintf(stderr, "CSIZE: %x\n", CSIZE);
    fprintf(stderr, "CS5: %x\n", CS5);
    fprintf(stderr, "CS6: %x\n", CS6);
    fprintf(stderr, "CS7: %x\n", CS7);
    fprintf(stderr, "CS8: %x\n", CS8);
    fprintf(stderr, "CSTOPB: %x\n", CSTOPB);
    fprintf(stderr, "CREAD: %x\n", CREAD);
    fprintf(stderr, "PARENB: %x\n", PARENB);
    fprintf(stderr, "PARODD: %x\n", PARODD);
    fprintf(stderr, "HUPCL: %x\n", HUPCL);
    fprintf(stderr, "CLOCAL: %x\n", CLOCAL);
    fprintf(stderr, "CIBAUD: %x\n", CIBAUD);
    fprintf(stderr, "CMSPAR: %x\n", CMSPAR);
    fprintf(stderr, "CRTSCTS: %x\n", CRTSCTS);
}

TtyIO::TtyIO(const TtyConfig &config) : PacketIO("TtyIO"), closing_(false), my_process_id(time(NULL)) {
    open(config);

    if (is_opened()) {
        open_pipe();
    }
}

TtyIO::~TtyIO() {
    file_.close();
}

void TtyIO::close() {
    closing_.store(true);
}

bool TtyIO::is_opened() {
    return !closing_.load();
}

mp::optional<std::list<memory::Bytes>> TtyIO::get() {
    unique_lock<mutex> in_locker(in_lock_);

    while ((file_.fd() >= 0) && ((source_data_.empty()) || (source_data_.begin()->first != pending_sessions.front()))) {
        in_locker.unlock();
        poll_io();
        in_locker.lock();
    }

    if (source_data_.empty() || source_data_.begin()->first != pending_sessions.front()) {
        return mp::nullopt_t();
    }

    auto result = slice_segments(move(source_data_.begin()->second));
    pending_sessions.remove(pending_sessions.front());
    source_data_.erase(source_data_.begin());

    return {mp::inplace_t(), result};
}

bool TtyIO::put(list<memory::Bytes> &&obj) {
    if (!is_opened()) {
        return false;
    }

    vector<uint32_t> sizes;
    sizes.push_back(obj.size());
    for (auto &segment : obj) {
        sizes.push_back(segment.size());
    }
    obj.insert(obj.begin(), memory::Bytes(move(sizes)));

    lock_guard<mutex> out_locker(out_lock_);

    auto session_id = next_write_session_id;
    next_write_session_id = (next_write_session_id + 1) % UINT32_MAX;
    size_t total_size = 0;

    for (auto &segment : obj) {
        total_size += segment.size();
    }

    for (size_t offset = 0; offset < total_size; offset += MaxDataLength) {
        queue_output(out_locker, session_id, TtyHeader::DATA, total_size, offset, min(total_size - offset, MaxDataLength));
    }

    write_sessions_[session_id].total_size = total_size;
    write_sessions_[session_id].pending.ranges = {math::Range{0, total_size}};
    write_sessions_[session_id].data = move(obj);

    return true;
}

void TtyIO::open(const TtyConfig &config) {
    file_.close();

    int fd = ::open(config.device.data(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        fprintf(stderr, "Cannot open tty device: %s\n", config.device.data());
        return;
    }

    FileIO tmp_fd(fd);

    struct termios old_settings;
    struct termios new_settings = config.settings();

    if(tcgetattr(tmp_fd.fd(), &old_settings) != 0) {
        fprintf(stderr, "Cannot get settings of tty device: %s\n", config.device.data());
        return;
    }

    if (tcsetattr(tmp_fd.fd(), TCSANOW, &new_settings) != 0) {
        fprintf(stderr, "Cannot set settings of tty device: %s\n", config.device.data());
        tcsetattr(tmp_fd.fd(), TCSANOW, &old_settings);
        return;
    }

    if(tcgetattr(tmp_fd.fd(), &old_settings) == 0) {
        fprintf(stderr, "iflag: expect: %x, actual: %x\n", new_settings.c_iflag, old_settings.c_iflag);
        fprintf(stderr, "cflag: expect: %x, actual: %x\n", new_settings.c_cflag, old_settings.c_cflag);
        fprintf(stderr, "lflag: expect: %x, actual: %x\n", new_settings.c_lflag, old_settings.c_lflag);
        fprintf(stderr, "oflag: expect: %x, actual: %x\n", new_settings.c_oflag, old_settings.c_oflag);
        fprintf(stderr, "in_baud: expected: %d, actual: %d\n", cfgetispeed(&new_settings), cfgetispeed(&old_settings));
        fprintf(stderr, "out_baud: expected: %d, actual: %d\n", cfgetospeed(&new_settings), cfgetospeed(&old_settings));
    } else {
        fprintf(stderr, "cannot get termios setting\n");
    }

    tcflush(tmp_fd.fd(), TCIOFLUSH);
    file_ = move(tmp_fd);
    closing_.store(false);
}

void TtyIO::open_pipe() {
    int pipefd[2];

    if (pipe(pipefd) == 0) {
        pipe_[0].set_fd(pipefd[0]);
        pipe_[1].set_fd(pipefd[1]);
    }
}

void TtyIO::poll_io() {
    if (closing_.load()) {
        fprintf(stderr, "closing tty\n");
        file_.close();
        return;
    }

    if (file_.buffered_read_data()) {
        handle_read();
    }

    struct pollfd subject[2];
    unique_lock<mutex> out_locker(out_lock_);
    subject[0].events = POLLIN | ((!output_queue_.empty() || file_.buffered_write_data()) ? POLLOUT : 0);
    out_locker.unlock();
    subject[0].fd = file_.fd();
    subject[1].events = POLLIN;
    subject[1].fd = pipe_[0].fd();
    char buffer;
    constexpr size_t hangup_threshold = 100;

    if (poll(subject, 2, 100) > 0) {
        if (subject[1].revents & POLLIN) {
            int ret = read(pipe_[0].fd(), &buffer, 1);
        }
        if (subject[0].revents & POLLIN) {
            hangup_test_ = 0;
            handle_read();
        }
        if (subject[0].revents & POLLOUT) {
            hangup_test_ = 0;
            if (file_.buffered_write_data()) {
                file_.flush_write();
            } else {
                handle_write();
            }
        }
    } else if (subject[0].revents & (POLLHUP | POLLRDHUP)) {
        close();
    } else {
        subject[0].events = POLLOUT;
        int ret;
        if ((ret = poll(subject, 1, 0)) > 0) {
            handle_write();
        } else {
            hangup_test_++;
            if (hangup_test_ > hangup_threshold) {
                close();
            }
            fprintf(stderr, "can't either read or write %d %x %x %lu\n", ret, subject[0].revents, POLLHUP, hangup_test_);
        }
    }
}

void TtyIO::handle_read() {
    TtyHeader header;
    if (!read_header(header)) {
        return;
    }

    {
        lock_guard<mutex> in_locker(in_lock_);
        if (peer_process_id != header.process_id) {
            peer_process_id = header.process_id;
            pending_sessions.ranges.clear();
            fprintf(stderr, "Peer is restarted\n");
        }
        if (pending_sessions.empty() && header.min_write_session_id != UINT32_MAX) {
            pending_sessions.ranges = {math::Range{header.min_write_session_id, UINT32_MAX}};
        }
    }

    switch (header.type) {
    case TtyHeader::DATA:
        handle_data(header);
        break;
    case TtyHeader::ACK:
        handle_ack(header);
        break;
    case TtyHeader::REQUEST:
        handle_request(header);
        break;
    }
}

void TtyIO::generate_output() {
    lock_guard<mutex> out_locker(out_lock_);

    if (output_queue_.empty()) {
        lock_guard<mutex> in_locker(in_lock_);

        for (auto &session : write_sessions_) {
            for (auto &range : session.second.pending.ranges) {
                for (size_t offset = range.begin; offset < range.end; offset += MaxDataLength) {
                    queue_output(out_locker, session.first, TtyHeader::DATA, session.second.total_size, offset, min(range.end - offset, MaxDataLength));
                    return;
                }
            }
        }
        for (auto &session : read_sessions_) {
            for (auto &range : session.second.pending.ranges) {
                for (size_t offset = range.begin; offset < range.end; offset += MaxDataLength) {
                    queue_output(out_locker, session.first, TtyHeader::REQUEST, session.second.data.size(), offset, min(range.end - offset, MaxDataLength));
                    return;
                }
            }
        }
    }
}

void TtyIO::handle_write() {
    generate_output();

    unique_lock<mutex> out_locker(out_lock_);

    if (!output_queue_.empty()) {
        TtyHeader header = output_queue_.front();
        output_queue_.pop();
        out_locker.unlock();

        header.process_id = my_process_id;
        header.min_write_session_id = write_sessions_.empty() ? UINT32_MAX : write_sessions_.begin()->first;

        if (header.type == TtyHeader::DATA) {
            out_locker.lock();
            auto iter = write_sessions_.find(header.session_id);
            if (iter != write_sessions_.end()) {
                file_.write((unsigned char*)&header, sizeof(TtyHeader));
                if (header.type == TtyHeader::DATA) {
                    assert(header.data_length <= MaxDataLength);
                    assert(header.data_offset + header.data_length <= iter->second.total_size);
                    auto offset = header.data_offset;
                    auto length = header.data_length;
                    for (auto &segment : iter->second.data) {
                        if (length <= 0) {
                            break;
                        } else if (offset >= segment.size()) {
                            offset -= segment.size();
                        } else {
                            size_t write_length = min<size_t>(length, segment.size() - offset);
                            file_.write(reinterpret_cast<unsigned char*>(segment.data()) + offset, write_length);
                            length -= write_length;
                            offset = 0;
                        }
                    }
                }
            }
        } else if (header.type == TtyHeader::REQUEST) {
            lock_guard<mutex> in_locker(in_lock_);
            auto iter = read_sessions_.find(header.session_id);
            if (iter != read_sessions_.end()) {
                assert(header.data_length <= MaxDataLength);
                file_.write((unsigned char*)&header, sizeof(TtyHeader));
            }
        } else {
            file_.write((unsigned char*)&header, sizeof(TtyHeader));
        }
    }
}

bool TtyIO::read_header(TtyHeader &header) {
    size_t offset = read_buffer_.size();
    read_buffer_.resize(offset + sizeof(TtyHeader));

    int bytes_read = file_.read(read_buffer_.data() + offset, 0, read_buffer_.size() - offset);
    if (bytes_read < 0) {
        read_buffer_.resize(offset);
        return false;
    }
    read_buffer_.resize(offset + bytes_read);

    if (read_buffer_.size() < sizeof(TtyHeader)) {
        return false;
    }

    auto search_end = read_buffer_.end() - sizeof(TtyHeader) + TtyHeader::MagicLength::value;
    auto ret = search(read_buffer_.begin() + offset - sizeof(TtyHeader), search_end,
                      TtyHeader::header_magic, TtyHeader::header_magic + TtyHeader::MagicLength::value);
    if (ret == search_end) {
        return false;
    } else {
        offset = distance(read_buffer_.begin(), ret);
        assert(read_buffer_.size() >= offset + sizeof(TtyHeader));
        memcpy(&header, read_buffer_.data() + offset, sizeof(TtyHeader));
        file_.unread(read_buffer_.data() + offset + sizeof(TtyHeader),  read_buffer_.size() - offset - sizeof(TtyHeader));
        read_buffer_.clear();
        return true;
    }
}

map<uint32_t, ReadSession>::iterator TtyIO::create_read_session(const TtyHeader &header) {
    auto iter = read_sessions_.find(header.session_id);
    if (iter == read_sessions_.end()) {
        iter = read_sessions_.emplace(header.session_id, ReadSession{}).first;
        iter->second.data.resize(header.total_length);
        iter->second.pending.ranges = {math::Range{0, header.total_length}};
    }
    return iter;
}

void TtyIO::handle_data(const TtyHeader &header) {
    lock_guard<mutex> in_locker(in_lock_);

    if (!pending_sessions.contains(header.session_id) || source_data_.count(header.session_id) > 0) {
        unsigned char buffer[MaxDataLength];
        file_.read(buffer, min<uint32_t>(header.data_length, MaxDataLength));
        queue_output(header.session_id, TtyHeader::ACK, header.total_length, 0, header.total_length);
        return;
    }

    auto iter = create_read_session(header);
    int bytes_read = file_.read(iter->second.data.data() + header.data_offset, min<size_t>(header.data_length, iter->second.data.size() - header.data_offset));
    iter->second.pending -= math::Range{header.data_offset, header.data_offset + bytes_read};

    queue_output(header.session_id, TtyHeader::ACK, header.total_length, header.data_offset, bytes_read);
    if (iter->second.pending.ranges.empty()) {
        source_data_.emplace(iter->first, move(iter->second.data));
        read_sessions_.erase(iter);
    }
}

void TtyIO::handle_ack(const TtyHeader &header) {
    lock_guard<mutex> out_locker(out_lock_);

    auto iter = write_sessions_.find(header.session_id);
    if (iter != write_sessions_.end()) {
        iter->second.pending -= math::Range{header.data_offset, header.data_offset + header.data_length};
        if (iter->second.pending.ranges.empty()) {
            write_sessions_.erase(iter);
        }
    }
}

void TtyIO::handle_request(const TtyHeader &header) {
    lock_guard<mutex> out_locker(out_lock_);

    auto iter = write_sessions_.find(header.session_id);
    if (iter != write_sessions_.end()) {
        queue_output(out_locker, header.session_id, TtyHeader::DATA, header.total_length, header.data_offset, header.data_length);
    }
}

void TtyIO::queue_output(lock_guard<mutex> &, uint32_t session_id, TtyHeader::PacketType type, uint32_t total_length, uint32_t data_offset, uint32_t data_length) {
    output_queue_.emplace();
    set_magic(output_queue_.back());
    output_queue_.back().session_id = session_id;
    output_queue_.back().type = type;
    output_queue_.back().total_length = total_length;
    output_queue_.back().data_offset = data_offset;
    output_queue_.back().data_length = data_length;

    unsigned char b;
    pipe_[1].write(&b, 1);
}

void TtyIO::queue_output(uint32_t session_id, TtyHeader::PacketType type, uint32_t total_length, uint32_t data_offset, uint32_t data_length) {
    lock_guard<mutex> out_locker(out_lock_);
    queue_output(out_locker, session_id, type, total_length, data_offset, data_length);
}

list<memory::Bytes> TtyIO::slice_segments(vector<unsigned char> &&input) {
    memory::Bytes data(std::move(input));
    auto count = memory::Iterable<uint32_t>(data.slice_front(sizeof(uint32_t)));
    auto sizes = memory::Iterable<uint32_t>(data.slice_front(sizeof(uint32_t) * (*count.begin())));
    list<memory::Bytes> segments;
    for (auto &size : sizes) {
        segments.emplace_back(data.slice_front(size));
    }
    return segments;
}
