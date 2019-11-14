#include "usb_comm.hpp"
#include "logging.hpp"
#include <cassert>
#include <cstdlib>

using namespace std;

static void *reader_thread_exec(void *p) {
    (reinterpret_cast<usb_io::Protocol*>(p))->reader_thread_exec();
    return nullptr;
}

static void *writer_thread_exec(void *p) {
    (reinterpret_cast<usb_io::Protocol*>(p))->writer_thread_exec();
    return nullptr;
}

namespace usb_io {

ostream &operator<<(ostream &s, const vector<uint32_t> &v) {
    s << "[" << v[0];
    for (size_t i = 1; i < v.size(); i++) {
        s << ", " << v[i];
    }
    s << "]";
    return s;
}

constexpr size_t MAX_TRANSFER_LENGTH = 32 * 1024;
constexpr size_t TIMEOUT_MILLISECONDS = 1000 * 5;
constexpr chrono::seconds RETRY_SYNC_INTERVAL(1);
constexpr chrono::seconds GET_WRITE_DATA_TIMEOUT(1);

constexpr array<unsigned char, Header().magic.size()> header_magic{{'B', 'M', '_', 'L', 'I', 'B', 'U', 'S', 'B', '_', 'T', 'R', 'A', 'N', 'S', 0}};

static void set_header_magic(Header &header) {
    memcpy(header.magic.data(), header_magic.data(), header_magic.size());
}

static bool validate_header_magic(const Header &header) {
    return memcmp(header.magic.data(), header_magic.data(), header_magic.size()) == 0;
}

Session::Session() :
PacketIO("usb_io::Session"),
read_pipe(0, data_pipe_mode::blocking),
write_pipe(0, data_pipe_mode::blocking) {
    opened.store(true);
}

Session::~Session() {
}

bool Session::is_opened() {
    return opened.load();
}

void Session::close() {
    read_pipe.close();
    opened.store(false);
}

mp::optional<packet_t> Session::get() {
    return read_pipe.get();
}

bool Session::put(packet_t &&packet) {
    if (opened.load()) {
        return write_pipe.put(move(packet));
    } else {
        return false;
    }
}

Protocol::Protocol() : Protocol(unique_ptr<Device>(new Device())) {}

Protocol::Protocol(unique_ptr<Device> &&dev) :
output_header_queue(0, data_pipe_mode::blocking),
device(move(dev)) {
    memset(&reader_thread, 0, sizeof(pthread_t));
    memset(&writer_thread, 0, sizeof(pthread_t));
    start();
}

Protocol::~Protocol() {
    LOGD << "destructing usb_io::Protocol";
    close();

    pthread_join(reader_thread, nullptr);
    pthread_join(writer_thread, nullptr);
}

void Protocol::reader_thread_exec() {
    auto last_success = chrono::steady_clock::now();
    try {
        while (is_opened()) {
            if (handle_read()) {
                last_success = chrono::steady_clock::now();
            } else if (session_state.load().state != State::Established && chrono::steady_clock::now() - last_success > RETRY_SYNC_INTERVAL) {
                lock_guard<mutex> locker(session_mutex);
                Header syn_header;
                syn_header.type = Header::Type::Syn;
                syn_header.session_id = session_id.load();
                output_header_queue.put(move(syn_header));
                last_success = chrono::steady_clock::now();
            }
        }
    } catch (exception &e) {
        LOGE << "exception in reader thread: " << e.what();
    }
}

void Protocol::writer_thread_exec() {
    try {
        while (is_opened()) {
            handle_write();
        }
    } catch (exception &e) {
        LOGE << "exception in writer thread: " << e.what();
    }
}

void Protocol::start() {
    create_session();

    pthread_create(&reader_thread, nullptr, ::reader_thread_exec, this);
    pthread_create(&writer_thread, nullptr, ::writer_thread_exec, this);
}

bool Protocol::is_opened() {
    return device->is_opened();
}

void Protocol::close() {
    device->close();
    lock_guard<mutex> locker(session_mutex);
    if (current_session) {
        current_session->close();
    }
}

shared_ptr<Session> Protocol::get_session() {
    lock_guard<mutex> locker(session_mutex);
    return current_session;
}

bool Protocol::rx_transfer(void *ptr, size_t size) {
    bool ok;
    size_t length;
    void *rx_ptr;
    array<unsigned char, sizeof(Header)> buffer;
    if (size < sizeof(Header)) {
        rx_ptr = buffer.data();
    } else {
        rx_ptr = ptr;
    }
    tie(ok, length) = device->read(reinterpret_cast<unsigned char*>(rx_ptr), max(size, sizeof(Header)), TIMEOUT_MILLISECONDS);
    if (length == sizeof(Header) && validate_header_magic(*reinterpret_cast<Header*>(buffer.data()))) {
        input_header_queue.emplace();
        memcpy(&input_header_queue.back(), rx_ptr, sizeof(Header));
        LOGD << "header recieved in reading data transfer";
        return false;
    } else if (length == size) {
        if (rx_ptr != ptr) {
            memcpy(ptr, rx_ptr, size);
        }
        return true;
    } else {
        LOGD << "read expect " << size << " bytes, recieved " << length << " bytes";
        return false;
    }
}

bool Protocol::handle_read() {
    Header input_header;
    if (input_header_queue.empty()) {
        return read_header(input_header) && handle_input_header(input_header);
    } else {
        input_header = input_header_queue.front();
        input_header_queue.pop();
        return handle_input_header(input_header);
    }
}

bool Protocol::handle_input_header(Header &input_header) {
    auto current_state = session_state.load();

    if (current_state.session_id != input_header.session_id && current_state.session_id != 0) {
        create_session();
        return true;
    }

    if (input_header.type == Header::Type::Syn || input_header.type == Header::Type::SynAck || input_header.type == Header::Type::Ack) {
        LOGD << "read header " << input_header.type << " " << static_cast<int>(current_state.state);
        Header resp_header;
        resp_header.session_id = session_id.load();

        switch (input_header.type) {
        case Header::Type::Syn:
            resp_header.type = Header::Type::SynAck;
            output_header_queue.put(move(resp_header));
            if (static_cast<int>(current_state.state) < static_cast<int>(State::SynAckSent)) {
                session_state.store({State::SynAckSent, input_header.session_id});
            }
            break;
        case Header::Type::SynAck:
            resp_header.type = Header::Type::Ack;
            output_header_queue.put(move(resp_header));
            session_state.store({State::Established, input_header.session_id});
            break;
        case Header::Type::Ack:
            session_state.store({State::Established, input_header.session_id});
            break;
        }
        return true;
    } else if (input_header.type == Header::Type::Data) {
        if (current_state.state == State::Established) {
            return read_data(input_header);
        } else {
            LOGD << "recv data in session_state " << static_cast<int>(current_state.state);
        }
    } else {
        LOGE << "unsupported header type" << input_header.type;
    }

    return false;
}

bool Protocol::read_header(Header &input_header) {
    if (!device->read(reinterpret_cast<unsigned char*>(&input_header), sizeof(Header), TIMEOUT_MILLISECONDS).first) {
        LOGI << "failed reading header";
        return false;
    }

    if (!validate_header_magic(input_header)) {
        LOGI << "failed validating header";
        return false;
    }

    return true;
}

bool Protocol::read_data(Header &input_header) {
    bool ok;
    vector<uint32_t> segment_lengths(input_header.data_info.segment_count);
    if (input_header.data_info.segment_count <= input_header.data_info.segment_lengths.size()) {
        memcpy(segment_lengths.data(), input_header.data_info.segment_lengths.data(), sizeof(uint32_t) * input_header.data_info.segment_count);
    } else {
        ok = rx_transfer(reinterpret_cast<unsigned char*>(segment_lengths.data()), segment_lengths.size() * sizeof(uint32_t));
        if (!ok) {
            LOGI << "failed reading segment lengths";
            return false;
        }
    }

    vector<vector<unsigned char>> input_data(input_header.data_info.segment_count);
    for (size_t i = 0; i < input_data.size(); i++) {
        input_data[i].resize(segment_lengths[i]);
        for (size_t offset = 0; offset < input_data[i].size(); offset += MAX_TRANSFER_LENGTH) {
            size_t read_length = min<size_t>(MAX_TRANSFER_LENGTH, input_data[i].size() - offset);
            ok = rx_transfer(input_data[i].data() + offset, read_length);
            if (!ok) {
                LOGI << "failed read data of segment " << i << ": (" << offset << ", " << read_length << ") / " << segment_lengths[i];
                return false;
            }
        }
    }

    packet_t packet;
    for (auto &v : input_data) {
        packet.emplace_back(move(v));
    }

    int i = 0;
    for (auto &b : packet) {
        assert(segment_lengths[i++] == b.size());
    }

    return current_session->read_pipe.put(move(packet));
}

void Protocol::create_session() {
    lock_guard<mutex> locker(session_mutex);
    if (current_session) {
        current_session->close();
    }
    output_header_queue.clear();

    session_id.store(chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now().time_since_epoch()).count());
    session_state.store({State::SynSent, 0});
    current_session = make_shared<Session>();
    Header syn_header;
    syn_header.type = Header::Type::Syn;
    syn_header.session_id = session_id.load();
    output_header_queue.put(move(syn_header));
    LOGD << "create session";
}

bool Protocol::handle_write() {
    packet_t packet;

    auto control_header = output_header_queue.try_get();

    if (control_header.has_value()) {
        set_header_magic(control_header.value());
        bool ok;
        size_t length;
        do {
            tie(ok, length) = device->write(reinterpret_cast<unsigned char*>(&control_header.value()), sizeof(Header), TIMEOUT_MILLISECONDS);
            LOGD << "write control header: " << static_cast<int>(control_header.value().type);
        } while (!ok && is_opened());
        return ok;
    }

    if (session_state.load().state != State::Established) {
        return false;
    } else if (!get_output_data(packet)) {
        LOGD << "no data to be sent";
        return false;
    }

    return write_data(move(packet));
}

bool Protocol::get_output_data(packet_t &packet) {
    lock_guard<mutex> locker(session_mutex);
    auto packet_op = current_session->write_pipe.try_get(GET_WRITE_DATA_TIMEOUT);
    if (!packet_op.has_value()) {
        return false;
    }
    packet.swap(packet_op.value());

    return true;
}

bool Protocol::write_data(packet_t &&packet) {
    bool ok;
    size_t length;
    Header output_header;
    set_header_magic(output_header);
    output_header.type = Header::Data;
    output_header.session_id = session_id.load();
    output_header.data_info.segment_count = packet.size();

    vector<uint32_t> lengths;
    if (packet.size() <= output_header.data_info.segment_lengths.size()) {
        int i = 0;
        for (auto &segment : packet) {
            output_header.data_info.segment_lengths[i++] = segment.size();
        }
    } else {
        lengths.reserve(packet.size());
        for (auto &segment : packet) {
            lengths.push_back(segment.size());
        }
    }

    tie(ok, length) = device->write(reinterpret_cast<unsigned char*>(&output_header), sizeof(Header), TIMEOUT_MILLISECONDS);
    if (!ok) {
        LOGI << "failed writing header to usb";
        return false;
    }
    if (!lengths.empty()) {
        tie(ok, length) = device->write(reinterpret_cast<unsigned char*>(lengths.data()), sizeof(uint32_t) * lengths.size(), TIMEOUT_MILLISECONDS);
        if (!ok) {
            LOGI << "failed writing length data to usb";
            return false;
        }
    }

    for (auto &segment : packet) {
        for (size_t offset = 0; offset < segment.size(); offset += MAX_TRANSFER_LENGTH) {
            size_t write_length = min<size_t>(MAX_TRANSFER_LENGTH, segment.size() - offset);
            tie(ok, length) = device->write(reinterpret_cast<unsigned char*>(segment.data()) + offset, write_length, TIMEOUT_MILLISECONDS);
            if (!ok) {
                LOGI << "failed write data of segment " << ": (" << offset << ", " << write_length << ") / " << segment.size();
                return false;
            }
        }
    }

    LOGD << "data sent";
    return true;
}

Peer::Peer() :
PacketIO("usb_io::Peer")
{
    static shared_ptr<Protocol> p = make_shared<Protocol>();
    static shared_ptr<Session> s = p->get_session();
    protocol = p;
    session = s;
}

Peer::~Peer() {
}

bool Peer::is_opened() {
    return session->is_opened() && protocol->is_opened();
}

void Peer::close() {
    session->close();
    protocol->close();
}

mp::optional<packet_t> Peer::get() {
    return session->get();
}

bool Peer::put(packet_t &&packet) {
    return session->put(move(packet));
}

} // namespace usb_io
