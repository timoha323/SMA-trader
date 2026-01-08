#include <iostream>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <fstream>

inline std::mutex log_mutex;

inline std::ofstream& log_file() {
    static std::ofstream file("sma-trader.log.txt", std::ios::app);
    return file;
}

#define LOG(level, message) do { \
    std::lock_guard<std::mutex> lock(log_mutex); \
    time_t now = time(nullptr); \
    struct tm* timeinfo = localtime(&now); \
    std::cout << "[" << std::put_time(timeinfo, "%Y-%m-%d %H:%M:%S") << "] " \
              << "[" << level << "] " << message << std::endl; \
    if (log_file().is_open()) { \
        log_file() << "[" << std::put_time(timeinfo, "%Y-%m-%d %H:%M:%S") << "] " \
                   << "[" << level << "] " << message << std::endl; \
    } \
} while (0)

#define LOG_INFO(message) LOG("INFO", message)
#define LOG_WARNING(message) LOG("WARNING", message)
#define LOG_ERROR(message) LOG("ERROR", message)
