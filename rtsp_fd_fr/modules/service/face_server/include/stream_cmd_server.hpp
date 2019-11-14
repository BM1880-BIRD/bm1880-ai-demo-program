#pragma once

#include "thread_runner.hpp"
#include "conditional_runner.hpp"
#include "tty_io.hpp"
#include "pipeline_core.hpp"
#include "stream_cmd_utils.hpp"
#include "config_file.hpp"
#include "repository.hpp"
#include "fdfr.hpp"
#include "packet_demux.hpp"
#include "image.hpp"
#include "mp/strong_typedef.hpp"

class StreamCmdServer {
public:
    StreamCmdServer();
    StreamCmdServer(std::string &&config_path);

    ~StreamCmdServer();

    virtual void init();
    virtual void startStreamPipeline(const std::string &stream_name);
    virtual void startCommandPipeline();
    virtual void run();
    virtual void mainLoop();
    virtual void stop();

    void setConfigPath(const std::string &config_path);
    void initDefaultCmd();
    void addCmd(const std::string &cmd, std::function<std::tuple<std::string>(std::tuple<std::string>)> f, std::string &&cmd_desp = "");
    void executeCmd(const std::string &cmd, const std::string &value, std::string &response);
    void setAsyncCapacity(const size_t capacity);
    bool registerFace(const std::string &name, cv::Mat &&image);
    template <typename T>
    void setContextValue(const std::string &key, T value) {
        context_->set<T>(key, std::move(value));
    }
    template <typename T>
    T getContextValue(const std::string &key) {
         return context_->get<T>(key)->value;
    }

protected:
    virtual void stopServer();

private:
    std::tuple<std::string, std::string> splitStrCmdValue(std::string &&str);

protected:
    std::shared_ptr<pipeline::Context> context_;
    TtyConfig tty_config_;
    ThreadRunner cmd_thread_;
    std::map<std::string, std::shared_ptr<ConditionalRunner>> stream_thread_map_;
    std::shared_ptr<TtyIO> tty_;
    std::string config_path_;
    MP_STRONG_TYPEDEF(std::string, cmd_switch_tag_t);
    std::shared_ptr<pipeline::DynamicSwitch<cmd_switch_tag_t, std::tuple<std::string &&>, std::tuple<std::string>>> cmd_switch_;
    std::map<std::string, std::string> cmd_map_;
    ConfigFile config_;
    std::shared_ptr<Repository> repo_;
    std::shared_ptr<PacketDemux> demux_;
    bool running_flag_;
    size_t async_capacity_;
};
