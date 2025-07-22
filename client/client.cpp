#include "../tools/price_generator/price_generator.h"
#include "../tools/logger/logger.h"

#include <cstring>
#include <iostream>
#include <fstream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

using namespace std;

namespace {
    constexpr const char* FILE_PREFIX = "file_to_send";
    constexpr int FILE_COUNT = 10000;
}

void sendFile(int socket, const string& filename) {
    ifstream file(filename, ios::binary);
    if (!file) {
        LOG_ERROR("Failed to open file: " << filename);
        return;
    }

    file.seekg(0, ios::end);
    uint32_t fileSize = file.tellg();
    file.seekg(0, ios::beg);

    uint32_t nameLength = filename.size();
    send(socket, &nameLength, sizeof(nameLength), 0);

    send(socket, filename.c_str(), nameLength, 0);

    send(socket, &fileSize, sizeof(fileSize), 0);

    char buffer[1024];
    while (file.read(buffer, sizeof(buffer)))
        send(socket, buffer, sizeof(buffer), 0);

    if (file.gcount() > 0)
        send(socket, buffer, file.gcount(), 0);

    LOG_INFO("Sent file: " << filename);
}

int main() {
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);

    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("connect");
        return 1;
    }

    for (int i = 0; i < FILE_COUNT; ++i) {
        string filename = string(FILE_PREFIX) + to_string(i);
        generateDataFile(filename);
        sendFile(clientSocket, filename);
    }

    shutdown(clientSocket, SHUT_WR);

    char response[1024] = {0};
    recv(clientSocket, response, sizeof(response), 0);
    LOG_INFO("Server replied: " << response);

    close(clientSocket);
    return 0;
}
