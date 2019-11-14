#pragma once

#include "thread_runner.hpp"
#include "tty_io.hpp"
#include "packet_demux.hpp"
#include "pipeline_core.hpp"
#include "stream_cmd_utils.hpp"

class StreamCmdClient {
public:
    StreamCmdClient();
    ~StreamCmdClient();

    virtual void startStreamPipeline(const std::string &stream_name);
    virtual void run();
    virtual void stop();

    bool sendCmd(const std::string &cmd, std::string &response);
    void addStream(std::string &&stream_name);
    void printUsage();
    void setContextValue(const std::string &key, std::string &&value);
    std::string getContextValue(const std::string &key);

protected:
    std::map<std::string, std::shared_ptr<ThreadRunner>> stream_thread_map_;
    std::shared_ptr<TtyIO> tty_;
    std::vector<std::string> stream_vec_;
    bool running_flag;
    std::shared_ptr<PacketDemux> demux_;
    std::shared_ptr<pipeline::Context> context_;
};
