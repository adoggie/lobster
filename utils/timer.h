#ifndef _LOB_TIMER_H
#define  _LOB_TIMER_H

#include <iostream>
#include <functional>
#include <chrono>
#include <thread>
#include <atomic>

class LobTimer {
public:
    LobTimer() : running_(false) {}

    void start(uint32_t timeout,std::function<void()> callback) {
        if (running_) {
            std::cout << "LobTimer is already running." << std::endl;
            return;
        }

        running_ = true;
        while (running_) {
            auto start = std::chrono::high_resolution_clock::now();
            callback();
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            if (duration.count() < timeout) {
                std::this_thread::sleep_for(std::chrono::milliseconds(timeout - duration.count()));
            } else {
                // std::cout << "Execution time exceeds 10ms." << std::endl;
            }
        }
    }

    void stop() {
        running_ = false;
    }

private:
    std::atomic<bool> running_;
};


#endif