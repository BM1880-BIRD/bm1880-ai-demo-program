#pragma once

#include <string>
#include <memory>
#include <vector>
#include <termios.h>
#include <mutex>
#include <map>
#include <condition_variable>
#include <queue>
#include "file_io.hpp"
#include "thread_runner.hpp"
#include "data_sink.hpp"
#include "data_source.hpp"
#include "packet.hpp"
#include "lru_cache.hpp"
#include "math/range.hpp"
#include "memory/bytes.hpp"

struct TtyHeader {
    typedef std::integral_constant<size_t, 8> MagicLength;
    static const char header_magic[MagicLength::value];
    enum PacketType : uint32_t {
        DATA = 1,
        ACK = 2,
        REQUEST = 3,
    };

    char magic[MagicLength::value];
    uint32_t process_id;
    uint32_t min_write_session_id;
    uint32_t session_id;
    PacketType type;
    uint32_t total_length;
    uint32_t data_offset;
    uint32_t data_length;

    bool operator==(const TtyHeader &rhs) const {
        return
            (session_id == rhs.session_id) &&
            (type == rhs.type) &&
            (total_length == rhs.total_length) &&
            (data_offset == rhs.data_offset) &&
            (data_length == rhs.data_length);
    }
};

static_assert(std::is_pod<TtyHeader>::value, "Packet header has to be POD");

void set_magic(TtyHeader &header);

struct TtyConfig {
    struct termios settings() const;

    std::string device;
    int baud;
};

struct ReadSession {
    math::RangeSet pending;
    std::vector<unsigned char> data;
};

struct WriteSession {
    size_t total_size;
    math::RangeSet pending;
    std::list<memory::Bytes> data;
};

class TtyIO : public PacketIO {
public:
    TtyIO(const TtyConfig &config);
    ~TtyIO();

    void close() override;
    bool is_opened() override;
    mp::optional<std::list<memory::Bytes>> get() override;
    bool put(std::list<memory::Bytes> &&obj) override;

private:
    void open(const TtyConfig &config);
    void open_pipe();
    void poll_io();
    void handle_read();
    void generate_output();
    void handle_write();
    bool read_header(TtyHeader &header);
    std::map<uint32_t, ReadSession>::iterator create_read_session(const TtyHeader &header);
    void handle_data(const TtyHeader &header);
    void handle_ack(const TtyHeader &header);
    void handle_request(const TtyHeader &header);
    void queue_output(std::lock_guard<std::mutex> &, uint32_t session_id, TtyHeader::PacketType type, uint32_t total_length, uint32_t data_offset, uint32_t data_length);
    void queue_output(uint32_t session_id, TtyHeader::PacketType type, uint32_t total_length, uint32_t data_offset, uint32_t data_length);
    std::list<memory::Bytes> slice_segments(std::vector<unsigned char> &&data);

    std::atomic_bool closing_;

    std::mutex in_lock_;
    const uint32_t my_process_id;
    uint32_t peer_process_id = UINT32_MAX;
    std::vector<unsigned char> read_buffer_;
    std::map<uint32_t, ReadSession> read_sessions_;
    math::RangeSet pending_sessions;
    std::map<uint32_t, std::vector<unsigned char>> source_data_;

    std::mutex out_lock_;
    size_t next_write_session_id = 1;
    std::map<uint32_t, WriteSession> write_sessions_;
    std::queue<TtyHeader> output_queue_;

    size_t hangup_test_ = 0;
    FileIO file_;
    FileIO pipe_[2];
};
