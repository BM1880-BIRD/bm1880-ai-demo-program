#pragma once

#include <memory>
#include "data_sink.hpp"
#include "data_source.hpp"

template <typename From, typename To>
class DataConversion {
public:
    typedef From input_type;
    typedef To output_type;
};

template <typename T>
class ConvertedSink : public DataSink<typename T::input_type> {
public:
    ConvertedSink(std::shared_ptr<DataSink<typename T::output_type>> &&sink) : sink_(std::move(sink)) {}
    bool put(typename T::input_type &&obj) override {
        typename T::output_type tmp;
        T::convert(std::move(obj), tmp);
        return sink_->put(std::move(tmp));
    }
    void close() override {
        sink_->close();
    }
    bool is_opened() override {
        return sink_->is_opened();
    }
private:
    std::shared_ptr<DataSink<typename T::output_type>> sink_;
};

template <typename T>
class ConvertedSource : public DataSource<typename T::output_type> {
public:
    ConvertedSource(std::shared_ptr<DataSource<typename T::input_type>> &&src) : src_(std::move(src)) {}
    bool get(typename T::output_type &obj) override {
        typename T::input_type tmp;
        if (!src_->get(tmp)) {
            return false;
        }
        T::convert(std::move(tmp), obj);
        return true;
    }
    bool is_opened() override {
        return src_->is_opened();
    }
    void close() override {
        src_->close();
    }
private:
    std::shared_ptr<DataSource<typename T::input_type>> src_;
};



template <typename Conversion, typename T>
std::shared_ptr<DataSink<typename Conversion::input_type>> make_convert(T &&sink, typename std::enable_if<std::is_base_of<DataSink<typename Conversion::output_type>, typename T::element_type>::value>::type *t = 0) {
    return std::make_shared<ConvertedSink<Conversion>>(std::move(sink));
}

template <typename Conversion, typename T>
std::shared_ptr<DataSource<typename Conversion::output_type>> make_convert(T &&src, typename std::enable_if<std::is_base_of<DataSource<typename Conversion::input_type>, typename T::element_type>::value>::type *t = 0) {
    return std::make_shared<ConvertedSource<Conversion>>(std::move(src));
}