#pragma once

#include <tuple>
#include "data_io.hpp"

template <typename... T>
class DataSink : public virtual DataIO {
public:
    using sink_type = DataSink<T...>;
    using input_type = std::tuple<T...>;

    virtual ~DataSink() {}

    virtual bool put(T &&...) = 0;
};