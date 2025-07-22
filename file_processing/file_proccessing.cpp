#include "file_proccessing.h"

#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>

PriceFile::PriceFile(const std::string& fileName) {
    std::ifstream file(fileName);
    std::string line;

    if (!file.is_open()) {
        std::cerr << "Opening file exception" << std::endl;
    }

    while (std::getline(file, line)) {
        timeToPrice[getTimestamp(line)] = getPrice(line);
    }

    file.close();
}

double PriceFile::getPrice(const std::string& line) {
    size_t priceIndexBegin = line.find(":");
    if (priceIndexBegin == std::string::npos) {
        throw std::invalid_argument("Delimiter ':' not found in the input string");
    }
    priceIndexBegin += 1;

    while (priceIndexBegin < line.size() && std::isspace(line[priceIndexBegin])) {
        ++priceIndexBegin;
    }
    if (priceIndexBegin >= line.size()) {
        throw std::invalid_argument("No price data found after ':'");
    }
    try {
        return std::stod(line.substr(priceIndexBegin));
    } catch (const std::invalid_argument&) {
        throw std::invalid_argument("Invalid price format after ':'");
    } catch (const std::out_of_range&) {
        throw std::out_of_range("Price value out of range for double");
    }
}

unsigned long long PriceFile::getTimestamp(const std::string& line) {
    size_t timestampIndexEnd = line.find(":");
    if (timestampIndexEnd == std::string::npos) {
        throw std::invalid_argument("Delimiter ':' not found in the input string");
    }

    size_t end = timestampIndexEnd;
    while (end > 0 && std::isspace(line[end - 1])) {
        --end;
    }
    if (end == 0) {
        throw std::invalid_argument("No timestamp data found before ':'");
    }
    try {
        return std::stoull(line.substr(0, end));
    } catch (const std::invalid_argument&) {
        throw std::invalid_argument("Invalid timestamp format before ':'");
    } catch (const std::out_of_range&) {
        throw std::out_of_range("Timestamp value out of range for unsigned long long");
    }
}

