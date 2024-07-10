#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <cstdlib>

#define BUFFER_SIZE 1024

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <port>" << std::endl;
        return EXIT_FAILURE;
    }

    const char* server_ip = argv[1];
    int port = std::stoi(argv[2]);
    if (port <= 0) {
        std::cerr << "Invalid port number." << std::endl;
        return EXIT_FAILURE;
    }

    int sock = 0;
    sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    // Create socket file descriptor
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return EXIT_FAILURE;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        return EXIT_FAILURE;
    }

    // Connect to the server
    if (connect(sock, reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        return EXIT_FAILURE;
    }

    std::string message;
    while (true) {
        std::cout << "Enter message to send to server: ";
        std::getline(std::cin, message);

        // Send the message to the server
        send(sock, message.c_str(), message.size(), 0);

        // Read the echoed message from the server
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_read = read(sock, buffer, BUFFER_SIZE);
        if (bytes_read <= 0) {
            std::cerr << "Read failed or connection closed by server" << std::endl;
            break;
        }
        std::cout << "Message from server: " << buffer << std::endl;
    }

    close(sock);
    return 0;
}

