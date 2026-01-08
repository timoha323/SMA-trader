#include "../tools/ThreadPool/ThreadPool.h"
#include "../tools/logger/logger.h"
#include "../tools/SMA/price_buffer.h"
#include "../file_processing/file_proccessing.h"

#include <algorithm>
#include <cstring>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
using socket_t = SOCKET;
constexpr socket_t invalid_socket = INVALID_SOCKET;
inline int close_socket(socket_t s) { return closesocket(s); }
inline int init_sockets() {
    WSADATA wsa{};
    return WSAStartup(MAKEWORD(2, 2), &wsa);
}
inline void cleanup_sockets() { WSACleanup(); }
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using socket_t = int;
constexpr socket_t invalid_socket = -1;
inline int close_socket(socket_t s) { return close(s); }
inline int init_sockets() { return 0; }
inline void cleanup_sockets() {}
#endif

using namespace std;

bool readBytes(socket_t socket, char* buffer, size_t size) {
    size_t total = 0;
    while (total < size) {
        int bytes = recv(socket, buffer + total, static_cast<int>(size - total), 0);
        if (bytes <= 0) return false;
        total += bytes;
    }
    return true;
}

uint32_t readUInt32(socket_t socket) {
    uint32_t value = 0;
    if (!readBytes(socket, reinterpret_cast<char*>(&value), sizeof(value))) {
        return 0;
    }
    return ntohl(value);
}

int main() {
    int counter = 1;
    if (init_sockets() != 0) {
        LOG_ERROR("Failed to initialize sockets");
        return 1;
    }

    socket_t serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == invalid_socket) {
        LOG_ERROR("Failed to create server socket");
        cleanup_sockets();
        return 1;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        LOG_ERROR("Failed to bind server socket");
        close_socket(serverSocket);
        cleanup_sockets();
        return 1;
    }
    if (listen(serverSocket, 5) < 0) {
        LOG_ERROR("Failed to listen on server socket");
        close_socket(serverSocket);
        cleanup_sockets();
        return 1;
    }
    LOG_INFO("Server is running on port 8080...");

    LOG_INFO("Start thread pool on 5 threads");
    ThreadPool threadPool(10);
    PriceBuffer priceBuffer;
    atomic<int> position(0); // long = 1; short = -1; neutral = 0
    atomic<double> balance(100'000.0);
    atomic<int> actives(10);
    const string outputDir = "files";
    std::error_code fsError;
    std::filesystem::create_directories(outputDir, fsError);
    if (fsError) {
        LOG_ERROR("Failed to create output directory: " + outputDir);
        close_socket(serverSocket);
        cleanup_sockets();
        return 1;
    }

    std::thread predictionThread([&priceBuffer, &position, &balance, &actives]() {
        int tries = 0;
        const int maxTriesToPredict = 8;
        double lastPrice = -1.0;
        auto goSleep = [](int tries, int maxTriesToPredict) {
            if (tries < maxTriesToPredict) {
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            } else {
                int shift = tries - maxTriesToPredict;
                auto delay = std::chrono::microseconds(1 << (shift > 10 ? 10 : shift));
                std::this_thread::sleep_for(delay);
            }
        };

        while (true) {
            std::this_thread::sleep_for(std::chrono::microseconds(50));

            if (priceBuffer.empty()) {
                goSleep(tries, maxTriesToPredict);
                continue;
            }
            double currentPrice = priceBuffer.getCurrentPrice();
            if (lastPrice == currentPrice) {
                goSleep(tries, maxTriesToPredict);
                continue;
            }
            lastPrice = currentPrice;
            double sma = priceBuffer.getAverage();

            LOG_INFO("Prediction thread: Current price = " 
                    + std::to_string(currentPrice) 
                    + ", SMA = " + std::to_string(sma)
                    + ", BALANCE = " + std::to_string(balance.load()));

            if (currentPrice > sma && position.load() <= 0) {
                //sell();
                if (actives.load() > 0) {
                    LOG("EVENT", "Selling");
                    double expected = balance.load();
                    double desired;
                    do {
                        desired = expected + currentPrice;
                    } while (!balance.compare_exchange_weak(expected, desired));
                    position.store(-1);
                    actives.fetch_sub(1);
                }
            }
            else if (currentPrice < sma && position.load() >= 0) {
                //buy();
                LOG("EVENT", "Buying");
                
                double expected = balance.load();
                double desired;
                do {
                    desired = expected - currentPrice;
                } while (!balance.compare_exchange_weak(expected, desired));
                position.store(1);
                actives.fetch_add(1);
            }

        }
    });


    while (true) {
        socket_t clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == invalid_socket) {
            LOG_ERROR("Failed to accept client");
            continue;
        }

        while (true) {
            uint32_t nameLen = readUInt32(clientSocket);
            if (nameLen == 0) break;

            string fileName(nameLen, 0);
            if (!readBytes(clientSocket, &fileName[0], nameLen)) break;

            uint32_t fileSize = readUInt32(clientSocket);
            string fullPath = outputDir + "/" + to_string(counter);

            ofstream outFile(fullPath, ios::binary);
            if (!outFile) {
                LOG_ERROR("Failed to open output file: " + fullPath);
                break;
            }

            char buffer[1024];
            size_t total = 0;
            while (total < fileSize) {
                size_t toRead = min(sizeof(buffer), static_cast<size_t>(fileSize - total));
                int bytes = recv(clientSocket, buffer, static_cast<int>(toRead), 0);
                if (bytes <= 0) break;
                outFile.write(buffer, bytes);
                total += bytes;
            }
            outFile.close();

            LOG_INFO("Received file " + fileName + " as #" + to_string(counter));

            threadPool.enqueue([counter, fullPath, &priceBuffer]() {
                LOG_INFO("Task " + to_string(counter) + " started");
                PriceFile file(fullPath);
                for (const auto& timeAndPrice : file.timeToPrice) {
                    priceBuffer.push(timeAndPrice.second);
                }
                // LOG_INFO("Price Buffer updated. Current price: "
                //     + to_string(priceBuffer.getCurrentPrice())
                //     + ". Current SMA: "
                //     + to_string(priceBuffer.getAverage())
                //     + ".");
            });

            ++counter;
        }

        const char* reply = "All files received!";
        send(clientSocket, reply, static_cast<int>(strlen(reply)), 0);
        close_socket(clientSocket);
    }

    close_socket(serverSocket);
    cleanup_sockets();
    return 0;
}
