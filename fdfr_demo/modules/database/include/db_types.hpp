#pragma once
#include <stdint.h>
#include <array>

namespace database {

enum class data_type : uint32_t {
    char8,
    int8,
    float32,
};

template <data_type T>
struct data;

template <>
struct data<data_type::char8> {
    using type = char;
};
template <>
struct data<data_type::int8> {
    using type = char;
};
template <>
struct data<data_type::float32> {
    using type = float;
};

using entry_id_t = uint32_t;

struct data_page_t {
    static const char *page_magic() {
        return "bm_pipe_db_data";
    }

    char magic_number[16];
    uint32_t field_id;
    uint32_t field_data_type;
    uint32_t field_data_length;
    uint32_t entry_count;
};

}