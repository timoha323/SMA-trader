#include "price_buffer.h"

#include <cstddef>
#include <mutex>

PriceBuffer::PriceBuffer() : windowSize_(20)
{}

PriceBuffer::PriceBuffer(std::size_t windowSize) : windowSize_(windowSize)
{}

void PriceBuffer::push(double value) {
    SpinLockGuard spinlockGuard(onWriteMutex_);
    if (priceBuffer_.size() >= windowSize_) {
        sum_ -= priceBuffer_.front();
        priceBuffer_.pop_front();
    }
    priceBuffer_.push_back(value);
    sum_ += value;
}

double PriceBuffer::getAverage() {
    SpinLockGuard spinlockGuard(onWriteMutex_);
    return sum_ / priceBuffer_.size();
}

double PriceBuffer::getCurrentPrice() {
    SpinLockGuard spinlockGuard(onWriteMutex_);
    return priceBuffer_.back();
}

double PriceBuffer::getVWAP() {
    SpinLockGuard spinlockGuard(onWriteMutex_);
    double weightedSum = 0.0;
    double totalVolume = 0.0;
    double volume = 1.0;

    for (double price : priceBuffer_) {
        weightedSum += price * volume;
        totalVolume += volume;
    }

    return weightedSum / totalVolume;
}

bool PriceBuffer::empty() {
    SpinLockGuard spinlockGuard(onWriteMutex_);
    return priceBuffer_.empty();
}
