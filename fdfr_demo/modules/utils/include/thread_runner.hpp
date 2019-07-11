#pragma once

#include <thread>
#include <atomic>
#include <mutex>

class ThreadRunner {
public:
    ~ThreadRunner() {
        join();
    }

    template<typename FuncType>
    void start(FuncType &&func) {
        std::lock_guard<std::mutex> guard(thread_lock);
        if (thread.joinable()) {
            throw std::runtime_error("ThreadRunner is already in use");
        }

        running.store(true);

        thread = std::thread([this, func]() {
            while (running.load()) {
                func();
            }
        });
    }

    void stop() {
        running.store(false);
    }

    bool is_running() {
        return running.load();
    }

    void join() {
        std::lock_guard<std::mutex> thread_guard(thread_lock);
        if (thread.joinable()) {
            thread.join();
        }
    }

private:
    std::atomic_bool running;
    std::thread thread;
    std::mutex thread_lock;
};