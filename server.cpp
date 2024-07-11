/*
References:
https://www.geeksforgeeks.org/socket-programming-in-cpp/
https://try.stackoverflow.co
https://chatgpt.com
*/

#include <iostream>
#include <iomanip>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <cctype> // for std::isxdigit
#include <string>

#define DATA_BYTE 1024

void handleClient(int client_fd, sockaddr_in client_addr);
int initializeServer(int port);

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[1]);
    int server_fd = initializeServer(port);
    if (server_fd < 0)
    {
        return -1;
    }

    // std::cout << "Server listening on port " << port << std::endl;

    while (true)
    {
        sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr *)&client_addr, &client_addr_len);
        if (client_fd == -1)
        {
            std::cerr << "Failed to accept client: " << strerror(errno) << std::endl;
            continue;
        }

        std::thread client_thread(handleClient, client_fd, client_addr);
        client_thread.detach(); // Detach the thread to handle clients concurrently
    }

    close(server_fd);
    return 0;
}

int initializeServer(int port)
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        return -1;
    }

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        std::cerr << "Failed to bind: " << strerror(errno) << std::endl;
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 10) == -1)
    {
        std::cerr << "Failed to listen: " << strerror(errno) << std::endl;
        close(server_fd);
        return -1;
    }

    return server_fd;
}

void parseData(unsigned char buffer[], unsigned long long int num_bytes_read, char clientIP[], int clientPort)
{
    // std::clog << "\nbyte: " << num_bytes_read << " - ";

    // // printing bytes
    // for (int i = 0; i < num_bytes_read; ++i)
    // {
    //     std::clog << std::hex << std::setw(2) << std::setfill('0')
    //               << (static_cast<unsigned>(buffer[i]) & 0xFF) << " ";
    // }
    // std::clog << std::endl;

    int i = 0;

    while (i < num_bytes_read - 1)
    {

        // Parse TYPE (2 bytes)
        int IdxTypeB1 = i;
        int IdxTypeB2 = ++i;

        uint16_t type = (static_cast<unsigned int>(buffer[IdxTypeB1]) << 8) | static_cast<unsigned int>(buffer[IdxTypeB2]);
        // std::cout << "\nConcatenated result: " << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << type << std::endl;
        std::string type_str;
        switch (type)
        {
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

        // std::cout << type_str << std::endl;

        // Parse TYPE (4 bytes)
        uint32_t IdxLenB1 = ++i;
        uint32_t IdxLenB2 = ++i;
        uint32_t IdxLenB3 = ++i;
        uint32_t IdxLenB4 = ++i;

        uint32_t length = (static_cast<uint32_t>(buffer[IdxLenB1]) << 24) |
                          (static_cast<uint32_t>(buffer[IdxLenB2]) << 16) |
                          (static_cast<uint32_t>(buffer[IdxLenB3]) << 8) |
                          static_cast<uint32_t>(buffer[IdxLenB4]);

        // std::cout << "Concatenated result for length: " << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << length << std::endl;
        // std::cout << length << std::endl;

        // int i = 6;
        int offset = (length >= 4) ? 4 : length;

        //[127.0.0.1:5678] [Data] [5]
        std::cout << "[" << clientIP << ":" << clientPort << "] " << "[" << type_str << "] " << "[" << length << "] ";
        std::cout << "[";
        for (int j = ++i; j < i + offset; j++)
        {
            std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0')
                      << (static_cast<unsigned>(buffer[j]) & 0xFF);
            std::cout.flush();

            // keep print space between bytes
            if (j < i + offset - 1)
                std::cout << " ";
        }

        std::cout << "]" << std::endl;

        std::cout.flush();
        // jumping to next BLOB
        i = i + length;
        // std::cout << "i: " << i << std::endl;
    }
}

void handleClient(int client_fd, sockaddr_in client_addr)
{
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), clientIP, INET_ADDRSTRLEN);
    int clientPort = ntohs(client_addr.sin_port);

    unsigned char buffer[DATA_BYTE] = {0};
    unsigned long long int num_bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);


    while (num_bytes_read > 0)
    {

        buffer[num_bytes_read] = '\0'; // Ensure null-termination

        // std::cout << "Received message from " << clientIP << ":" << clientPort << " - " << buffer << std::endl;

        parseData(buffer, num_bytes_read, clientIP, clientPort);
        std::cout << std::endl;

        num_bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    }

    if (num_bytes_read == 0)
    {
        std::cout << "Client disconnected: " << clientIP << ":" << clientPort << std::endl;
    }
    else
    {
        std::cerr << "Failed to receive data: " << strerror(errno) << std::endl;
    }

    close(client_fd);
}
