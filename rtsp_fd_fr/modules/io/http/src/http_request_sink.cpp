#include "http_request_sink.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sstream>
#include "logging.hpp"

constexpr size_t RETRY_TIMES = 5;

static FILE *connect(const std::string &host_addr, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd == -1) {
        LOGI << "failed creating socket fd";
        return nullptr;
    }

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = PF_INET;
    addr.sin_addr.s_addr = inet_addr(host_addr.data());
    addr.sin_port = htons(port);

    if (connect(fd, (sockaddr*)&addr, sizeof(addr)) != 0){
        LOGI << "failed connecting to " << host_addr << ":" << port;
        ::close(fd);
        return nullptr;
    }

    FILE *fp = fdopen(fd, "w");
    if (fp == nullptr) {
        LOGI << "failed to fdopen";
        ::close(fd);
        return nullptr;
    }

    return fp;
}

static FILE *connect_retry(const std::string &host_addr, int port, int retry) {
    FILE *fp = nullptr;
    for (int i = 0; i < retry && fp == nullptr; i++) {
        if (i > 0) {
            LOGI << "retrying HTTP connection";
        }

        fp = connect(host_addr, port);
    }

    if (fp == nullptr) {
        LOGW << "failed to connect " << host_addr << ":" << port << " for " << retry << " retries.";
    }

    return fp;
}

bool http_post_content(const std::string &host_addr, int port, const std::string &url, const std::string &mimetype, const memory::Bytes &data) {
    FILE *fp = connect_retry(host_addr, port, RETRY_TIMES);
    if (fp == nullptr) {
        return false;
    }

    fprintf(fp, "POST %s HTTP/1.1\r\n", url.data());
    fprintf(fp, "Content-Type: %s\r\n", mimetype.data());
    fprintf(fp, "Content-Length: %lu\r\n", data.size());
    fprintf(fp, "\r\n");
    fwrite(data.data(), sizeof(unsigned char), data.size(), fp);
    fclose(fp);

    return true;
}

#define BOUNDARY            "simple boundary"
#define DELIMETER           "--" BOUNDARY
#define CRLF                "\r\n"
#define CONTENT_DISP_PRE    "Content-Disposition: form-data; name=\""
#define CONTENT_DISP_POST   "\"" CRLF
#define CONTENT_TYPE_PRE    "Content-Type: "
#define CONTENT_TYPE_POST   CRLF
#define TERMINAL            DELIMETER "--"


constexpr size_t cstrlen(const char *s) {
    return *s ? 1 + cstrlen(s + 1) : 0;
}

static size_t content_length(const std::vector<std::tuple<std::string, std::string, memory::Bytes>> &data) {
    constexpr size_t per_part_constant = cstrlen(CRLF) * 3 + cstrlen(DELIMETER CONTENT_DISP_PRE CONTENT_DISP_POST CONTENT_TYPE_PRE CONTENT_TYPE_POST);
    constexpr size_t final_constant = cstrlen(TERMINAL CRLF);

    size_t runtime_sum = 0;
    for (auto &part : data) {
        runtime_sum += std::get<0>(part).length() + std::get<1>(part).length() + std::get<2>(part).size();
    }

    return runtime_sum + per_part_constant * data.size() + final_constant;
}

bool http_post_multipart(const std::string &addr, int port, const std::string &url, const std::vector<std::tuple<std::string, std::string, memory::Bytes>> &content) {
    FILE *fp = connect_retry(addr, port, RETRY_TIMES);
    if (fp == nullptr) {
        return false;
    }

    fprintf(fp, "POST %s HTTP/1.1\r\n", url.data());
    fprintf(fp, "Content-Type: multipart/form-data; boundary=\"%s\"\r\n", BOUNDARY);
    fprintf(fp, "Content-Length: %lu\r\n", content_length(content));
    fprintf(fp, "%s", CRLF);

    for (auto &part : content) {
        fprintf(fp, "%s", CRLF DELIMETER CRLF);
        fprintf(fp, "%s%s%s", CONTENT_DISP_PRE, std::get<0>(part).data(), CONTENT_DISP_POST);
        fprintf(fp, "%s%s%s", CONTENT_TYPE_PRE, std::get<1>(part).data(), CONTENT_TYPE_POST);
        fprintf(fp, "%s", CRLF);
        auto &view = std::get<2>(part);
        fwrite(view.data(), 1, view.size(), fp);
    }

    fprintf(fp, "%s", CRLF TERMINAL);
    fclose(fp);

    return true;
}