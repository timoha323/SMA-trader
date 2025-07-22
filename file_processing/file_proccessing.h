#pragma once

#include <string>
#include <chrono>
#include <unordered_map>
#include <optional>

class PriceFile {
public:

    PriceFile(const std::string& fileName);
    ~PriceFile() = default;

    std::unordered_map<unsigned long long, double> timeToPrice;

private:
    double getPrice(const std::string& line);
    unsigned long long getTimestamp(const std::string& line);
};