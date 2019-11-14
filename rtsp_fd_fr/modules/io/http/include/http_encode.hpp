#pragma once

#include <vector>
#include "json.hpp"
#include "memory/bytes.hpp"
#include "vector_pack.hpp"

template <typename T>
struct HttpEncode;

template <>
struct HttpEncode<json> {
    static const char *mimetype() {
        return "application/json";
    }
    static memory::Bytes encode(json &&j) {
        auto s = j.dump();
        return memory::Bytes(std::vector<char>(s.begin(), s.end()));
    }
};

template <>
struct HttpEncode<VectorPack> {
public:
    static const char *mimetype() {
        return HttpEncode<json>::mimetype();
    }
    static memory::Bytes encode(const VectorPack &vp) {
        return HttpEncode<json>::encode(vp.get_json());
    }
};