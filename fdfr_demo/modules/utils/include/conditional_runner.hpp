#pragma once

#include <list>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

class ConditionalRunner {
public:
    ConditionalRunner() : current_state(states::paused) {}

    ~ConditionalRunner() {
        stop();
        join();
    }

    template<typename FuncType>
    void add_task(FuncType &&func) {
        std::lock_guard<std::mutex> thread_guard(thread_lock);

        threads.emplace_back([this, func]() {
            while (true) {
                if (current_state.load() == states::paused) {
                    std::unique_lock<std::mutex> state_guard(state_lock);
                    // Wait start flag to start
                    state_cv.wait(state_guard, [this] {
                        return current_state.load() != states::paused;
                    });
                }
                if (current_state.load() == states::stopped) {
                    break;
                }

                try {
                    func();
                } catch (std::exception &e) {
                    break;
                }
            }
        });
    }

    void start() {
        current_state.store(states::running);
        state_cv.notify_all();
    }

    void pause() {
        current_state.store(states::paused);
        state_cv.notify_all();
    }

    void stop() {
        current_state.store(states::stopped);
        state_cv.notify_all();
    }

    bool is_running() {
        return current_state.load() == states::running;
    }

    void join() {
        std::lock_guard<std::mutex> thread_guard(thread_lock);
        for (auto &thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        threads.clear();
    }

private:
    enum class states {
        running,
        paused,
        stopped,
    };

    std::mutex state_lock;
    std::atomic<states> current_state;
    std::condition_variable state_cv;
    std::mutex thread_lock;
    std::list<std::thread> threads;
};