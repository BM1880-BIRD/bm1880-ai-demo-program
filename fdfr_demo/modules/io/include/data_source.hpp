#pragma once

#include <tuple>
#include "data_io.hpp"
#include "optional.hpp"

template <typename... T>
class DataSource : public virtual DataIO {
public:
    using source_type = DataSource<T...>;
    using output_type = std::tuple<T...>;

    virtual ~DataSource() {}

    virtual mp::optional<T...> get() = 0;
};