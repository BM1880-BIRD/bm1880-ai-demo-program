#pragma once

#include <vector>
#include <memory>
#include "conditional_runner.hpp"
#include "pipeline_core.hpp"

namespace pipeline {

class Executor {
public:
    void add_pipes(std::vector<std::shared_ptr<pipeline::Pipeline>> &&pipes) {
        for (auto &pipe : pipes) {
            runner.add_task([pipe]() {
                if (!pipe->execute_once()) {
                    pipe->close();
                    throw std::runtime_error("err");
                }
            });
        }

        pipes.clear();
    }

    void start() {
        runner.start();
    }

    void pause() {
        runner.pause();
    }

    void stop() {
        runner.stop();
    }

    bool is_running() {
        return runner.is_running();
    }

    void join() {
        runner.join();
    }

private:
    ConditionalRunner runner;
};

}