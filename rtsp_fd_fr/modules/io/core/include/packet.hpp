#pragma once

#include <list>
#include <memory>
#include <cstdint>
#include "file_io.hpp"
#include "data_sink.hpp"
#include "data_source.hpp"
#include "memory/bytes.hpp"

class PacketIO : public DataSource<std::list<memory::Bytes>>, public DataSink<std::list<memory::Bytes>> {
public:
    PacketIO(const char *name) : DataSource<std::list<memory::Bytes>>(name), DataSink<std::list<memory::Bytes>>(name) {}
    virtual ~PacketIO() {}
};