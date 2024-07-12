#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// Convert hex string to binary data
std::vector<char> hexToBinary(const std::string& hex) {
    std::vector<char> binaryData;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        char byte = static_cast<char>(strtol(byteString.c_str(), nullptr, 16));
        binaryData.push_back(byte);
    }
    return binaryData;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <server IP> <port>" << std::endl;
        return 1;
    }

    std::string serverIP = argv[1];
    int port = atoi(argv[2]);

    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation failed." << std::endl;
        return 1;
    }

    // Fill in server address information
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address / Address not supported." << std::endl;
        close(sock);
        return 1;
    }

    // Connect to server
    if (connect(sock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        std::cerr << "Connection Failed." << std::endl;
        close(sock);
        return 1;
    }

    std::string inputLine;
    while (std::getline(std::cin, inputLine)) {
        auto binaryData = hexToBinary(inputLine);
        send(sock, binaryData.data(), binaryData.size(), 0);
    }

    close(sock);
    return 0;
}
