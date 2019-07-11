#pragma once

#include "data_sink.hpp"
#include "memory_view.hpp"
#include <tuple>
#include <vector>
#include <memory>
#include <string>
#include "http_encode.hpp"

bool http_post_content(const std::string &addr, int port, const std::string &url, const std::string &mimetype, const Memory::SharedView<unsigned char> &content);
bool http_post_multipart(const std::string &addr, int port, const std::string &url, const std::vector<std::tuple<std::string, std::string, Memory::SharedView<unsigned char>>> &content);

template <typename T>
class HttpRequestSink : public DataSink<T> {
public:
    HttpRequestSink(const std::string &host, int port, const std::string &url) :
    host_(host),
    port_(port),
    url_(url) {}

    bool put(T &&data) override {
        return http_post_content(host_, port_, url_, HttpEncode<T>::mimetype(), HttpEncode<T>::encode(std::move(data)));
    }
    void close() override {}
    bool is_opened() override {
        return true;
    }

private:
    std::string host_;
    int port_;
    std::string url_;
};

template <typename... Ts>
class HttpMultipartSink : public DataSink<Ts...> {

public:
    HttpMultipartSink(const std::string &addr, int port, const std::string &url, std::vector<std::string> &&names) :
    addr_(addr),
    port_(port),
    url_(url),
    names_(std::move(names)) {}

    explicit HttpMultipartSink(const json &config) :
    HttpMultipartSink(
        config.at("ip").get<std::string>(),
        config.at("port").get<int>(),
        config.at("path").get<std::string>(),
        config.at("names").get<std::vector<std::string>>()
    ) {}

    bool put(Ts &&...args) override {
        std::vector<std::tuple<std::string, std::string, Memory::SharedView<unsigned char>>> data;
        encode(data, std::move(args)...);
        return http_post_multipart(addr_, port_, url_, data);
    }

    void close() override {}
    bool is_opened() override {
        return true;
    }

private:
    template <typename U, typename... Us>
    void encode(std::vector<std::tuple<std::string, std::string, Memory::SharedView<unsigned char>>> &out, U &&u, Us &&...us) {
        out.emplace_back(names_[out.size()], HttpEncode<U>::mimetype(), HttpEncode<U>::encode(std::move(u)));
        encode(out, std::move(us)...);
    }
    void encode(std::vector<std::tuple<std::string, std::string, Memory::SharedView<unsigned char>>> &out) {}

    std::string addr_;
    int port_;
    std::string url_;
    std::vector<std::string> names_;
};
