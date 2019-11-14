#pragma once

#include <fstream>
#include "vector_pack.hpp"
#include "data_sink.hpp"
#include "json.hpp"

class VectorPackDump : public DataSink<VectorPack> {
public:
    explicit VectorPackDump(const json &config) : VectorPackDump(config.at("filepath").get<std::string>()) {}
    explicit VectorPackDump(const std::string &filename) : DataSink<VectorPack>("VectorPackDump"), stream(filename) {}

    bool put(VectorPack &&vp) override {
        if (!is_opened()) {
            return false;
        }
        json output = vp.get_json();
        stream << output.dump() << std::endl;
        return true;
    }

    bool is_opened() override {
        return stream.is_open() && stream.good();
    }

    void close() override {
        stream.close();
    }

private:
    std::ofstream stream;
};
