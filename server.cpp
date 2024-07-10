#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <cstdlib>

#define BUFFER_SIZE 1024

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return EXIT_FAILURE;
    }

    int port = std::stoi(argv[1]);
    if (port <= 0) {
        std::cerr << "Invalid port number." << std::endl;
        return EXIT_FAILURE;
    }

    int server_fd, new_socket;
    sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return EXIT_FAILURE;
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0) {
        std::cerr << "Setsockopt failed" << std::endl;
        close(server_fd);
        return EXIT_FAILURE;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Bind the socket to the network address and port
    if (bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        close(server_fd);
        return EXIT_FAILURE;
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        std::cerr << "Listen failed" << std::endl;
        close(server_fd);
        return EXIT_FAILURE;
    }

    std::cout << "Server listening on port. . . " << port << std::endl;

    // Accept an incoming connection
    if ((new_socket = accept(server_fd, reinterpret_cast<sockaddr*>(&address), &addrlen)) < 0) {
        std::cerr << "Accept failed" << std::endl;
        close(server_fd);
        return EXIT_FAILURE;
    }

    // Continuous communication loop
    while (true) {
        std::cout << "waiting for message... " << std::endl;
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_read = read(new_socket, buffer, BUFFER_SIZE);
        if (bytes_read <= 0) {
            std::cerr << "Read failed or connection closed by client" << std::endl;
            break;
        }
        std::cout << "Message from client: " << buffer << std::endl;

        strcat(buffer," recieved");
        // Echo the message back to the client
        send(new_socket, buffer, strlen(buffer), 0);
    }

    close(new_socket);
    close(server_fd);
    return 0;
}


