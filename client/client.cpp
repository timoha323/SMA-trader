#include "../tools/price_generator/price_generator.h"
#include "../tools/logger/logger.h"

#include <cstring>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

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
#include <arpa/inet.h>
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

namespace {
    constexpr const char* FILE_DIR = "files_to_send";
    constexpr const char* FILE_PREFIX = "file_to_send";
    constexpr int FILE_COUNT = 2500;
}

void sendFile(socket_t socket, const string& filePath) {
    std::filesystem::path path(filePath);
    const string filename = path.filename().string();

    ifstream file(path, ios::binary);
    if (!file) {
        LOG_ERROR("Failed to open file: " << filePath);
        return;
    }

    file.seekg(0, ios::end);
    uint32_t fileSize = static_cast<uint32_t>(file.tellg());
    file.seekg(0, ios::beg);

    uint32_t nameLength = htonl(static_cast<uint32_t>(filename.size()));
    send(socket, reinterpret_cast<const char*>(&nameLength), sizeof(nameLength), 0);

    uint32_t nameLengthHost = ntohl(nameLength);
    send(socket, filename.c_str(), static_cast<int>(nameLengthHost), 0);

    uint32_t fileSizeNet = htonl(fileSize);
    send(socket, reinterpret_cast<const char*>(&fileSizeNet), sizeof(fileSizeNet), 0);

    char buffer[1024];
    while (file.read(buffer, sizeof(buffer)))
        send(socket, buffer, static_cast<int>(sizeof(buffer)), 0);

    if (file.gcount() > 0)
        send(socket, buffer, static_cast<int>(file.gcount()), 0);

    LOG_INFO("Sent file: " << filename);
}

int main() {
    if (init_sockets() != 0) {
        LOG_ERROR("Failed to initialize sockets");
        return 1;
    }

    socket_t clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == invalid_socket) {
        perror("socket");
        cleanup_sockets();
        return 1;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);

    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("connect");
        close_socket(clientSocket);
        cleanup_sockets();
        return 1;
    }

    std::error_code fsError;
    std::filesystem::create_directories(FILE_DIR, fsError);
    if (fsError) {
        LOG_ERROR("Failed to create directory: " << FILE_DIR);
        close_socket(clientSocket);
        cleanup_sockets();
        return 1;
    }

    for (int i = 0; i < FILE_COUNT; ++i) {
        std::filesystem::path filePath = std::filesystem::path(FILE_DIR) / (string(FILE_PREFIX) + to_string(i));
        string pathStr = filePath.string();
        generateDataFile(pathStr);
        sendFile(clientSocket, pathStr);
    }

    shutdown(clientSocket,
#ifdef _WIN32
        SD_SEND
#else
        SHUT_WR
#endif
    );

    char response[1024] = {0};
    recv(clientSocket, response, static_cast<int>(sizeof(response)), 0);
    LOG_INFO("Server replied: " << response);

    close_socket(clientSocket);
    cleanup_sockets();
    return 0;
}
