#pragma once

#include <mutex>
#include <queue>
#include <map>
#include <string>
#include <memory>
#include <condition_variable>
#include "thread_runner.hpp"
#include "packet.hpp"

class PacketDemux;

struct DemuxHeader {
    enum Type : uint32_t {
        Control = 1,
        Data = 2,
    };
    Type type;
    uint32_t buffer_capacity;
};

class DemuxStream : public PacketIO {
    friend class PacketDemux;
public:
    DemuxStream(const std::string &name, std::shared_ptr<PacketIO> io, std::shared_ptr<PacketDemux> owner, uint32_t buffer_capacity);

    mp::optional<std::vector<Memory::SharedView<unsigned char>>> get() override;
    bool is_opened() override;
    bool put(std::vector<Memory::SharedView<unsigned char>> &&data) override;
    void close() override;

protected:
    void push_read_data(std::vector<Memory::SharedView<unsigned char>> &&data);
    void set_peer_capacity(uint32_t peer_capacity);

private:
    bool send(const DemuxHeader &header, std::vector<Memory::SharedView<unsigned char>> &&data);

    std::string name_;
    Memory::SharedView<unsigned char> name_data_;
    std::shared_ptr<PacketIO> io_;
    std::shared_ptr<PacketDemux> owner_;

    std::mutex read_mutex_;
    uint32_t buffer_capacity_;
    std::condition_variable read_avail_;
    std::queue<std::vector<Memory::SharedView<unsigned char>>> read_data_;

    std::mutex write_mutex_;
    std::condition_variable write_avail_;
    uint32_t peer_capacity_;
};

class PacketDemux : public std::enable_shared_from_this<PacketDemux> {
public:
    PacketDemux(std::shared_ptr<PacketIO> io);
    ~PacketDemux();

    void start();
    void stop();

    std::shared_ptr<PacketIO> create_stream(const std::string &key, uint32_t capacity = 1);
    void close_stream(const std::string &key);
    bool is_opened(const std::string &key);


private:
    void demux();

    std::mutex mutex_;
    std::map<std::string, std::shared_ptr<DemuxStream>> streams_;
    std::shared_ptr<PacketIO> io_;
    ThreadRunner demux_thread_;
};
