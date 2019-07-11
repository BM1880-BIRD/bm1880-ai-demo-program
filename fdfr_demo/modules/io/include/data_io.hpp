#pragma once

class DataIO {
public:
    virtual ~DataIO() {}

    virtual bool is_opened() = 0;
    virtual void close() = 0;
};