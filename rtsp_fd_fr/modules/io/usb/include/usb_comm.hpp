#pragma once

#include <memory>
#include <vector>
#include <stdint.h>
#include <pthread.h>
#include <array>
#include <atomic>
#include <mutex>
#include <list>
#include "memory/bytes.hpp"
#include "usb_device.hpp"
#include "packet.hpp"
#include "data_pipe.hpp"
#include "packet.hpp"

namespace usb_io {

using packet_t = std::list<memory::Bytes>;

struct Header {
    enum Type : uint32_t {
        Syn = 1,
        SynAck,
        Ack,
        Data,
    };

    std::array<unsigned char, 16> magic;
    Type type;
    uint32_t session_id;
    union {
        struct {
            uint32_t segment_count;
            std::array<uint32_t, 16> segment_lengths;
        } data_info;
    };
};

class Protocol;

/**
 * \class usb_io::Session
 */
class Session : public PacketIO {
    friend class Protocol;
public:
    Session();
    ~Session();

    /**
     * check if this Session is opened
     * @return true if opened
     */
    bool is_opened() override;
    /// close this session
    void close() override;
    /**
     * get a packet from input message queue
     * @return an optional packet with value; no value if session is closed
     */
    mp::optional<packet_t> get() override;
    /**
     * put a packet into output message queue
     * @return true if sueccess, false if session is closed
     */
    bool put(packet_t &&) override;

private:
    std::atomic_bool opened;
    DataPipe<packet_t> read_pipe;
    DataPipe<packet_t> write_pipe;
};

/**
 * \class usb_io::Protocol
 * usb_io::Protocol transfers data in packets via libusb device.
 * A packet is a list of memory::Bytes.
 *
 * After handshaking, usb_io::Protocol creates an usb_io::Session instance to control transmission.
 * If the Session on either side is closed, another session needs to be created in order to transfer packets.
 */
class Protocol {
    friend class Session;
public:
    /// construct a Protocol with default constructed usb_io::Device
    Protocol();
    Protocol(std::unique_ptr<Device> &&dev);
    ~Protocol();

    bool is_opened();
    void close();
    /**
     * @return current working session, returned session may be closed but won;t be null
     */
    std::shared_ptr<Session> get_session();
    void reader_thread_exec();
    void writer_thread_exec();

private:
    enum class State {
        SynSent = 1,
        SynAckSent,
        Established,
    };

    void start();
    bool rx_transfer(void *ptr, size_t size);
    bool handle_read();
    bool handle_input_header(Header &input_header);
    bool read_header(Header &input_header);
    bool read_data(Header &input_header);
    void create_session();
    bool handle_write();
    bool get_output_data(packet_t &packet);
    bool write_data(packet_t &&packet);

private:
    struct session_state_t {
        State state;
        uint32_t session_id;
    };

    std::atomic<session_state_t> session_state;
    std::atomic<uint32_t> session_id;
    std::mutex session_mutex;
    std::shared_ptr<Session> current_session;

    pthread_t reader_thread;
    pthread_t writer_thread;

    std::queue<Header> input_header_queue;
    DataPipe<Header> output_header_queue;
    std::unique_ptr<Device> device;
};

class Peer : public PacketIO {
public:
    Peer();
    ~Peer();

    bool is_opened() override;
    void close() override;
    mp::optional<packet_t> get() override;
    bool put(packet_t &&) override;

private:
    std::shared_ptr<Protocol> protocol;
    std::shared_ptr<Session> session;
};

}
