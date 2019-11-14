#pragma once

#include <iostream>
#include "pipe_component.hpp"

class ConsoleCaptureSource : public DataSource<std::string> {
public:
    ConsoleCaptureSource() : DataSource<std::string>("ConsoleCatureSource") {}
    ~ConsoleCaptureSource() {
    }

    mp::optional<std::string> get() override {
        std::string value;
        getline(std::cin, value);
        return {mp::inplace_t(), std::move(value)};
    }

    bool is_opened() override {
        return true;
    }

    void close() override {}
};

template <typename T>
class ConsoleOutPipeComp : public pipeline::Component<const T &> {
public:
    ConsoleOutPipeComp() {prefix_ = "";}
    ConsoleOutPipeComp(std::string &&prefix) : prefix_(move(prefix)) {}

    std::tuple<> process(std::tuple<const T &> args) {
        std::cout << prefix_ << std::get<0>(args) << std::endl;
        return {};
    }

private:
    std::string prefix_;
};
