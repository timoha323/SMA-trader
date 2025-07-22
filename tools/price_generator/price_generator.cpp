#include "price_generator.h"
#include "../logger/logger.h"

#include <fstream>
#include <random>
#include <ctime>

void generateDataFile(const std::string& filename) {
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        LOG_ERROR("Fail to open generating file");
        return;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> timeDist(1721571720, 1721572720);
    std::uniform_real_distribution<double> priceDist(9.0, 15.0);

    for (int i = 0; i < 200; ++i) {
        int timestamp = timeDist(gen);
        double price = priceDist(gen);
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.2f", price);
        outFile << timestamp << ":" << buffer << "\n";
    }

    outFile.close();
    LOG_INFO("Price generated");
}
