#pragma once

#include <tuple>
#include <string>
#include <memory>
#include "pipe_component.hpp"
#include "pipe_context.hpp"

template <typename T>
class PipeContextReader : public pipeline::Component<> {
public:
    PipeContextReader(const std::string &key) : key_(key) {}

    std::tuple<T> process(std::tuple<> ) {
        if (value_ == nullptr) {
            value_ = this->context_->template get<T>(key_);
            if (value_ == nullptr) {
                throw std::runtime_error("PipeContextReader: cannot read variable[" + key_ + "] from pipeline::Context");
            }
        }

        value_->mutex.lock();
        auto tmp = std::make_tuple(value_->value);
        value_->mutex.unlock();

        return tmp;
    }

private:
    std::shared_ptr<pipeline::Context::Variable<T>> value_;
    std::string key_;
};