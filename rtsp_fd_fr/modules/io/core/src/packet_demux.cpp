#include "packet_demux.hpp"
#include <iterator>
#include <chrono>
#include "memory/encoding.hpp"

using namespace std;

DemuxStream::DemuxStream(const std::string &name, std::shared_ptr<PacketIO> io, std::shared_ptr<PacketDemux> owner, uint32_t buffer_capacity) :
PacketIO("DemuxStream"),
name_(name), name_data_(vector<unsigned char>(name.begin(), name.end())), io_(io), owner_(owner), buffer_capacity_(buffer_capacity), peer_capacity_(1) {}

mp::optional<list<memory::Bytes>> DemuxStream::get() {
    unique_lock<mutex> locker(read_mutex_);

    read_avail_.wait(locker, [this]() {
        return !is_opened() || !read_data_.empty();
    });

    if (read_data_.empty()) {
        return mp::nullopt_t();
    }

    auto result = std::move(read_data_.front());
    read_data_.pop();

    DemuxHeader header;
    header.type = DemuxHeader::Control;
    header.buffer_capacity = buffer_capacity_ - read_data_.size();

    send(header, {});

    return {mp::inplace_t(), std::move(result)};
}

bool DemuxStream::is_opened() {
    return owner_->is_opened(name_);
}

bool DemuxStream::put(list<memory::Bytes> &&data) {
    unique_lock<mutex> locker(write_mutex_);

    write_avail_.wait(locker, [this]() {
        return !is_opened() || peer_capacity_ > 0;
    });

    if (peer_capacity_ == 0) {
        return false;
    }

    DemuxHeader header;
    header.type = DemuxHeader::Data;

    peer_capacity_--;
    return send(header, std::move(data));
}

void DemuxStream::close() {
    unique_lock<mutex> read_locker(read_mutex_);
    unique_lock<mutex> write_locker(write_mutex_);

    owner_->close_stream(name_);
    read_avail_.notify_all();
    write_avail_.notify_all();
}

void DemuxStream::push_read_data(list<memory::Bytes> &&data) {
    unique_lock<mutex> locker(read_mutex_);

    read_data_.emplace(move(data));
    read_avail_.notify_one();
}

void DemuxStream::set_peer_capacity(uint32_t peer_capacity) {
    unique_lock<mutex> locker(write_mutex_);

    if (peer_capacity_ == 0 && peer_capacity > 0) {
        write_avail_.notify_all();
    }
    peer_capacity_ = peer_capacity;
}

bool DemuxStream::send(const DemuxHeader &header, std::list<memory::Bytes> &&data) {
    data.insert(data.begin(), name_data_);
    vector<char> v(sizeof(header));
    memcpy(v.data(), &header, sizeof(header));
    data.insert(next(data.begin()), memory::Bytes(move(v)));
    return io_->put(std::move(data));
}

PacketDemux::PacketDemux(shared_ptr<PacketIO> io) : io_(io) {
}

PacketDemux::~PacketDemux() {
    io_->close();
    demux_thread_.stop();
    demux_thread_.join();
}

void PacketDemux::start() {
    demux_thread_.start([this]() {
        demux();
    });
}

void PacketDemux::stop() {
    unique_lock<mutex> locker(mutex_);
    auto copy = move(streams_);
    locker.unlock();

    for (auto &pair : copy) {
        pair.second->close();
    }

    locker.lock();
    io_->close();
    demux_thread_.stop();
}

shared_ptr<PacketIO> PacketDemux::create_stream(const string &key, uint32_t capacity) {
    unique_lock<mutex> locker(mutex_);

    auto iter = streams_.find(key);
    if (iter == streams_.end()) {
        iter = streams_.emplace(key, make_shared<DemuxStream>(key, io_, shared_from_this(), capacity)).first;
    }

    return iter->second;
}

void PacketDemux::close_stream(const string &key) {
    unique_lock<mutex> locker(mutex_);

    streams_.erase(key);
}

bool PacketDemux::is_opened(const std::string &key) {
    if (!io_->is_opened()) {
        return false;
    }
    unique_lock<mutex> locker(mutex_);

    return streams_.count(key) != 0;
}

void PacketDemux::demux() {
    auto data = io_->get();

    if (!data.has_value()) {
        stop();
        return;
    }

    if (!data->empty()) {
        string key;
        memory::Encoding<string>::decode(data.value(), key);

        unique_lock<mutex> locker(mutex_);
        auto iter = streams_.find(key);
        if (iter == streams_.end() || iter->second == nullptr) {
            return;
        }

        if (!data->empty()) {
            auto header = memory::Iterable<DemuxHeader>(data->front().slice_front(sizeof(DemuxHeader)));
            data->erase(data->begin());

            if (header.size() == 1) {
                auto stream = iter->second;
                locker.unlock();

                switch (header.begin()->type) {
                case DemuxHeader::Control:
                    stream->set_peer_capacity(header.begin()->buffer_capacity);
                    break;
                case DemuxHeader::Data:
                    stream->push_read_data(move(data.value()));
                    break;
                }
            }
        }
    }
}
