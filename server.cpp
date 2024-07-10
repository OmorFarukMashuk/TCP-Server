#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>

#define BUFFER_SIZE 1024

// Function to convert type to a readable string
std::string typeToString(uint16_t type) {
    switch (type) {
        case 0xE110: return "Hello";
        case 0xDA7A: return "Data";
        case 0x0B1E: return "Goodbye";
        default: return "Unknown";
    }
}

// Function to format the first 4 bytes of value in hex
std::string formatHex(const char* value, uint32_t length) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint32_t i = 0; i < length && i < 4; ++i) {
        if (i > 0) oss << " ";
        oss << "0x" << std::setw(2) << static_cast<int>(static_cast<unsigned char>(value[i]));
    }
    return oss.str();
}

void handle_client(int client_socket, sockaddr_in client_address) {
    char buffer[BUFFER_SIZE];
    char client_ip[INET_ADDRSTRLEN];

    // Convert the client's IP address to a string
    inet_ntop(AF_INET, &(client_address.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_address.sin_port);

    std::cout << "Client connected: " << client_ip << ":" << client_port << std::endl;

    while (true) {
        std::memset(buffer, 0, BUFFER_SIZE);
        int bytes_read = read(client_socket, buffer, BUFFER_SIZE);
        if (bytes_read <= 0) {
            if (bytes_read == 0) {
                std::cerr << "Client disconnected: " << client_ip << ":" << client_port << std::endl;
            } else {
                std::cerr << "Read failed for client: " << client_ip << ":" << client_port << std::endl;
            }
            break;
        }

        int offset = 0;
        while (offset < bytes_read) {
            if (bytes_read - offset < 6) {
                std::cerr << "Invalid TLV format from client: " << client_ip << ":" << client_port << std::endl;
                break;
            }

            // Parse TYPE (2 bytes)
            uint16_t type;
            std::memcpy(&type, buffer + offset, 2);
            type = ntohs(type);
            offset += 2;

            // Parse LENGTH (4 bytes)
            uint32_t length;
            std::memcpy(&length, buffer + offset, 4);
            length = ntohl(length);
            offset += 4;

            // Check if the remaining bytes match the declared length
            if (bytes_read - offset < static_cast<int>(length)) {
                std::cerr << "Invalid LENGTH field from client: " << client_ip << ":" << client_port << std::endl;
                break;
            }

            // Parse VALUE (variable length)
            std::vector<uint8_t> value(buffer + offset, buffer + offset + length);
            offset += length;

            // Determine the type of message and print it
            std::string type_str;
            switch (type) {
                case 0xE110:
                    type_str = "Hello";
                    break;
                case 0xDA7A:
                    type_str = "Data";
                    break;
                case 0x0B1E:
                    type_str = "Goodbye";
                    break;
                default:
                    type_str = "Unknown";
                    break;
            }

            std::cout << "[" << client_ip << ":" << client_port << "] [" << type_str << "] [" << length << "] [";
            for (size_t i = 0; i < value.size(); ++i) {
                if (i > 0) {
                    std::cout << " ";
                }
                std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(value[i]);
            }
            std::cout << "]" << std::endl;
        }
    }

    close(client_socket);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return EXIT_FAILURE;
    }

    int port = std::stoi(argv[1]);
    if (port <= 0)
    {
        std::cerr << "Invalid port number." << std::endl;
        return EXIT_FAILURE;
    }

    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        std::cerr << "Socket creation failed" << std::endl;
        return EXIT_FAILURE;
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0)
    {
        std::cerr << "Setsockopt failed" << std::endl;
        close(server_fd);
        return EXIT_FAILURE;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Bind the socket to the network address and port
    if (bind(server_fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0)
    {
        std::cerr << "Bind failed" << std::endl;
        close(server_fd);
        return EXIT_FAILURE;
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0)
    {
        std::cerr << "Listen failed" << std::endl;
        close(server_fd);
        return EXIT_FAILURE;
    }

    std::cout << "Server listening on port " << port << std::endl;

    // Accept incoming connections
    while (true)
    {
        sockaddr_in client_address;
        socklen_t client_addrlen = sizeof(client_address);
        int new_socket = accept(server_fd, reinterpret_cast<sockaddr *>(&client_address), &client_addrlen);
        if (new_socket < 0)
        {
            std::cerr << "Accept failed" << std::endl;
            close(server_fd);
            return EXIT_FAILURE;
        }

        std::thread client_thread(handle_client, new_socket, client_address);
        client_thread.detach();
    }

    close(server_fd);
    return 0;
}