/*
References:
https://www.geeksforgeeks.org/socket-programming-in-cpp/
https://try.stackoverflow.co
https://learn.microsoft.com/en-us/cpp/
*/

#include <iostream>
#include <iomanip>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <unordered_map>
#include <chrono>
#include <mutex>

#define DATA_BYTE 1030 // considering T - 2 BYTE, L - 4 BYTE, V - 1024 BYTE

class TCPServer
{
private:
    int port;
    int server_fd;
    std::unordered_map<std::string, std::pair<int, std::chrono::steady_clock::time_point>> rateLimiter;
    std::mutex rateMutex;

public:
    TCPServer(int port) : port(port)
    {
        server_fd = initializeServer();
        if (server_fd < 0)
        {
            throw std::runtime_error("Server initialization failed");
        }
    }

    ~TCPServer()
    {
        close(server_fd);
    }

    void run()
    {
        std::cout << "Server listening on port " << port << std::endl;
        while (true)
        {
            sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
            if (client_fd == -1)
            {
                std::cerr << "Failed to accept client: " << strerror(errno) << std::endl;
                continue;
            }

            std::thread([=]
                        { handleClient(client_fd, client_addr); })
                .detach();
        }
    }

private:
    int initializeServer()
    {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd == -1)
        {
            std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
            return -1;
        }

        sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        server_addr.sin_port = htons(port);

        if (bind(fd, (sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        {
            std::cerr << "Failed to bind: " << strerror(errno) << std::endl;
            close(fd);
            return -1;
        }

        if (listen(fd, 10) == -1)
        {
            std::cerr << "Failed to listen: " << strerror(errno) << std::endl;
            close(fd);
            return -1;
        }

        return fd;
    }
    void parseData(char buffer[], unsigned long long int num_bytes_read, char clientIP[], int clientPort)
    {
        int i = 0;

        // termination condition (1byte = 2 Hex)
        while (i < num_bytes_read - 1)
        {
            // Ensure there's enough data left for TYPE and LENGTH
            // if (num_bytes_read - i < 6) {
            //     std::cerr << "Incomplete TLV header from client: " << clientIP << ":" << clientPort << std::endl;
            //     break;
            // }
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

            if(type_str == "Unknown"){
            std::cout << "[" << clientIP << ":" << std::to_string(clientPort) << "] " << "[" << type_str << "] " << "[]" << "[Data Corruption Occurred] ";
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
            std::cout << length << std::endl;
            
            // Ensure there's size of VALUE matches LENGTH
            // if (num_bytes_read - i +1 != static_cast<int>(length)) {
            //     std::cerr << "Invalid VALUE field from client: " << clientIP << ":" << std::to_string(clientPort) << std::endl;
            //     break;
            // }

            // Parse VALUE
            // int i = 6;
            int offset = (length >= 4) ? 4 : length;
            std::cout << "[" << clientIP << ":" << std::to_string(clientPort) << "] " << "[" << type_str << "] " << "[" << length << "] ";
            std::cout << "[";
            for (int j = ++i; j < i + offset; j++)
            {
                std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0')
                          << (static_cast<unsigned>(buffer[j]) & 0xFF);

                // keep print space between bytes
                if (j < i + offset - 1)
                    std::cout << " ";
            }

            std::cout << "]" << std::endl;

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

        char buffer[DATA_BYTE] = {0};
        unsigned long int num_bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

        while (num_bytes_read > 0)
        {
            buffer[num_bytes_read] = '\0'; // Ensure null-termination

            if (!checkRateLimit(clientIP))
            {
                std::cerr << "Rate limit exceeded for: " << clientIP << std::endl;
                break; // Disconnect client
            }

            std::clog << "Received data from " << clientIP << ":" << std::to_string(clientPort) << " - ";
            for (int i = 0; i < num_bytes_read; ++i)
            {
                std::clog << std::hex << std::setw(2) << std::setfill('0') << (static_cast<unsigned>(buffer[i]) & 0xFF) << " ";
            }
            std::clog << std::endl;

            std::clog << "\ntotal bytes read: " << num_bytes_read << std::endl;

            parseData(buffer, num_bytes_read, clientIP, clientPort);

            num_bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        }

        if (num_bytes_read == 0)
        {
            std::clog << "Client disconnected: " << clientIP << ":" << std::to_string(clientPort) << std::endl;
        }
        else
        {
            std::cerr << "Failed to receive data: " << strerror(errno) << std::endl;
        }

        close(client_fd);
    }

    bool checkRateLimit(const std::string &ip)
    {
        std::lock_guard<std::mutex> lock(rateMutex);
        auto &[count, last_time] = rateLimiter[ip];
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_time).count() < 10)
        {
            if (++count > 2)
            { // Allow max  requests per second per client
                return false;
            }
        }
        else
        {
            count = 1;
            last_time = now;
        }
        return true;
    }
};

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }
    int port = std::stoi(argv[1]);

    try
    {
        TCPServer server(port);
        server.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
