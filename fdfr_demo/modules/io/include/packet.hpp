#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include "file_io.hpp"
#include "data_sink.hpp"
#include "data_source.hpp"
#include "memory_view.hpp"

struct PacketHeader {
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

    bool operator==(const PacketHeader &rhs) const {
        return
            (session_id == rhs.session_id) &&
            (type == rhs.type) &&
            (total_length == rhs.total_length) &&
            (data_offset == rhs.data_offset) &&
            (data_length == rhs.data_length);
    }
};

static_assert(std::is_pod<PacketHeader>::value, "Packet header has to be POD");

void set_magic(PacketHeader &header);

class PacketIO : public DataSource<std::vector<Memory::SharedView<unsigned char>>>, public DataSink<std::vector<Memory::SharedView<unsigned char>>> {
public:
    virtual ~PacketIO() {}
};