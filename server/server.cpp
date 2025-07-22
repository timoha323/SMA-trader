#include "../tools/ThreadPool/ThreadPool.h"
#include "../tools/logger/logger.h"
#include "../tools/SMA/price_buffer.h"
#include "../file_processing/file_proccessing.h"

#include <cstring>
#include <iostream>
#include <fstream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

uint32_t readUInt32(int socket) {
    uint32_t value;
    recv(socket, &value, sizeof(value), MSG_WAITALL);
    return value;
}

bool readBytes(int socket, char* buffer, size_t size) {
    size_t total = 0;
    while (total < size) {
        ssize_t bytes = recv(socket, buffer + total, size - total, 0);
        if (bytes <= 0) return false;
        total += bytes;
    }
    return true;
}

int main() {
    int counter = 1;
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    listen(serverSocket, 5);
    LOG_INFO("Server is running on port 8080...");

    LOG_INFO("Start thread pool on 5 threads");
    ThreadPool threadPool(10);
    PriceBuffer priceBuffer;

    while (true) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket < 0) {
            LOG_ERROR("Failed to accept client");
            continue;
        }

        while (true) {
            uint32_t nameLen;
            ssize_t r = recv(clientSocket, &nameLen, sizeof(nameLen), MSG_WAITALL);
            if (r == 0 || r < 0) break;

            string fileName(nameLen, 0);
            if (!readBytes(clientSocket, &fileName[0], nameLen)) break;

            uint32_t fileSize = readUInt32(clientSocket);
            string fullPath = "files/" + to_string(counter);

            ofstream outFile(fullPath, ios::binary);
            if (!outFile) {
                LOG_ERROR("Failed to open output file: " + fullPath);
                break;
            }

            char buffer[1024];
            size_t total = 0;
            while (total < fileSize) {
                size_t toRead = min(sizeof(buffer), static_cast<size_t>(fileSize - total));
                ssize_t bytes = recv(clientSocket, buffer, toRead, 0);
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
                LOG_INFO("Price Buffer updated. Current price: "
                    + to_string(priceBuffer.getCurrentPrice())
                    + ". Current SMA: "
                    + to_string(priceBuffer.getAverage())
                    + ".");
            });

            ++counter;
        }

        const char* reply = "All files received!";
        send(clientSocket, reply, strlen(reply), 0);
        close(clientSocket);
    }

    close(serverSocket);
    return 0;
}
