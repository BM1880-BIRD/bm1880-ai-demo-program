#pragma once

#include <tuple>
#include <string>
#include "data_io.hpp"

template <typename... T>
class DataSink : public virtual DataIO {
public:
    using sink_type = DataSink<T...>;
    using input_type = std::tuple<T...>;

    DataSink(const char *name) : name(name) {}
    virtual ~DataSink() {}

    virtual bool put(T &&...) = 0;
    const char *sink_name() const {
        return name.data();
    }

private:
    std::string name;
};