#pragma once
#include "../spinlock/spinlock.h"
#include "../spinlock/spinlockGuard.h"

#include <cstddef>
#include <deque>
#include <mutex>

class PriceBuffer {
public:

    PriceBuffer();
    PriceBuffer(std::size_t windowSize);

    void push(double value);
    double getAverage();
    double getCurrentPrice();

    bool empty();

private:
    SpinLock onWriteMutex_;
    std::deque<double> priceBuffer_;
    const std::size_t windowSize_;
    double sum_ = 0;
};