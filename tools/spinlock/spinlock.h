#pragma once

#include <atomic>
#include <thread>
#include <chrono>

class SpinLock {
public:
    SpinLock() = default;

    void lock() {
        int retries = 0;
        while (flag.test_and_set()) {
            backoff(retries);
            ++retries;
        }
    }

    void unlock() {
        flag.clear();
    }

private:
    void backoff(int retries) {
        const int max_yield_retries = 8;
        if (retries < max_yield_retries) {
            std::this_thread::yield();
        } else {
            int shift = retries - max_yield_retries;
            auto delay = std::chrono::microseconds(1 << (shift > 10 ? 10 : shift));
            std::this_thread::sleep_for(delay);
        }
    }

    std::atomic_flag flag = ATOMIC_FLAG_INIT;
};
