#pragma once

#include "json.hpp"
#include "memory_view.hpp"

template <typename T>
struct HttpEncode;

template <>
struct HttpEncode<json> {
    static const char *mimetype() {
        return "application/json";
    }
    static Memory::SharedView<unsigned char> encode(json &&j) {
        return Memory::SharedView<unsigned char>(j.dump());
    }
};