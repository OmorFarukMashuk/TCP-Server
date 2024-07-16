/*
References:
https://www.geeksforgeeks.org/socket-programming-in-cpp/
https://try.stackoverflow.co
https://learn.microsoft.com/en-us/cpp/
*/

#include "server.h"

TCPServer::TCPServer(int port) : port(port)
{
    this->server_fd = initializeServer();
    if (this->server_fd < 0)
    {
        throw std::runtime_error("Server initialization failed");
    }
}

TCPServer::~TCPServer()
{
    close(this->server_fd);
}

void TCPServer::run()
{
    std::clog << "Server listening on port . . ." << port << std::endl;
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

        // Using thread to handle each client connection concurrently
        std::thread([=]
                    { handleClient(client_fd, client_addr); })
            .detach();
    }
}

int TCPServer::initializeServer()
{
    // Creating socket file descriptor
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        return -1;
    }

    // Set socket options and bind the socket to the network address and port
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

    // Listen for incoming connections
    if (listen(fd, 10) == -1)
    {
        std::cerr << "Failed to listen: " << strerror(errno) << std::endl;
        close(fd);
        return -1;
    }

    return fd;
}

void TCPServer::parseData(char buffer[], unsigned int num_bytes_read, char client_IP[], int client_port)
{
    unsigned int i = 0;

    // Termination condition (1 byte represents 2 Hex)
    while (i < num_bytes_read - 1)
    {
        // Parse TYPE (2 bytes)
        int idx_type_b1 = i;
        int idx_type_b2 = ++i;

        uint16_t type = (static_cast<unsigned int>(buffer[idx_type_b1]) << 8) | // Take the byte at idx_type_b1, convert it to uint16_t,
                                                                                // and shift left by 8 bits to place it in the highest byte
                        static_cast<unsigned int>(buffer[idx_type_b2]);         // Take the byte at idx_type_b2, convert it to uint16_t,
                                                                                // and place it in the lowest byte

        // std::clog << "\nConcatenated result: " << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << type << std::endl;
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

        // Checking data corruption
        if (type_str == "Unknown")
        {
            std::cerr << "[" << client_IP << ":" << std::to_string(client_port) << "] " << "[" << type_str << "] " << "[]" << "[Data Corruption Occurred]" << std::endl;
            break;
        }

        // std::clog << type_str << std::endl;

        // Parse TYPE (4 bytes)
        uint32_t idx_len_b1 = ++i;
        uint32_t idx_len_b2 = ++i;
        uint32_t idx_len_b3 = ++i;
        uint32_t idx_len_b4 = ++i;

        // The bitwise OR operation (|) is used to combine these four separate byte values into a single 32-bit integer
        uint32_t length = (static_cast<uint32_t>(buffer[idx_len_b1]) << 24) | // Take the byte at idx_len_b1, convert it to uint32_t,
                                                                              // and shift left by 24 bits to place it in the highest byte

                          (static_cast<uint32_t>(buffer[idx_len_b2]) << 16) | // Take the byte at idx_len_b2, convert it to uint32_t,
                                                                              // and shift left by 16 bits to place it in the second highest byte

                          (static_cast<uint32_t>(buffer[idx_len_b3]) << 8) | // Take the byte at idx_len_b3, convert it to uint32_t,
                                                                             // and shift left by 8 bits to place it in the third highest byte

                          static_cast<uint32_t>(buffer[idx_len_b4]); // Take the byte at idx_len_b4, convert it to uint32_t,
                                                                     // and place it in the lowest byte

        // std::clog << length << std::endl;

        // Parse VALUE
        int offset = (length >= BYTES_TO_READ) ? BYTES_TO_READ : length;
        std::cout << "[" << client_IP << ":" << std::to_string(client_port) << "] " << "[" << type_str << "] " << "[" << length << "] ";
        std::cout << "[";
        for (int j = ++i; j < i + offset; j++)
        {
            // Output a formatted hexadecimal value of a byte from the buffer
            std::cout << "0x"                                       // Prefix '0x' indicates hexadecimal format in the output
                      << std::hex                                   // Set the number base of the output stream to hexadecimal
                      << std::setw(2)                               // Set the width of the next output number to 2 characters
                      << std::setfill('0')                          // Set the fill character for padding to '0' if the output number has less than 2 digits
                      << (static_cast<unsigned>(buffer[j]) & 0xFF); // Convert the byte at buffer[j] to unsigned,
                                                                    // apply a mask to ensure it's a single byte (0xFF),
                                                                    // and output it as a hexadecimal value

            // keep print space between bytes
            if (j < i + offset - 1)
                std::cout << " ";
        }

        std::cout << "]" << std::endl;

        // jumping to next BLOB
        i = i + length;
    }
}

void TCPServer::handleClient(int client_fd, sockaddr_in client_addr)
{
    char client_IP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_IP, INET_ADDRSTRLEN);
    int client_port = ntohs(client_addr.sin_port);

    std::clog << "Client Connected: " << client_IP << ":" << std::to_string(client_port) << std::endl;

    char buffer[DATA_BYTE] = {0};
    unsigned int num_bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    while (num_bytes_read > 0)
    {
        buffer[num_bytes_read] = '\0'; // Ensure null-termination

        std::string client_port_str = std::to_string(client_port);
        std::string client_IP_str = client_IP;

        std::string full_ip = client_IP_str + ": " + client_port_str;

        if (!checkRateLimit(full_ip))
        {
            std::cerr << "Rate limit exceeded for: " << client_IP << ":" << client_port_str << std::endl;
            break; // Disconnect client
        }

        std::clog << "Received data from " << client_IP << ":" << std::to_string(client_port) << " - ";
        for (int i = 0; i < num_bytes_read; ++i)
        {
            std::clog << std::hex << std::setw(2) << std::setfill('0') << (static_cast<unsigned>(buffer[i]) & 0xFF) << " ";
        }
        std::clog << std::endl;

        // std::clog << "\ntotal bytes read: " << num_bytes_read << std::endl;

        parseData(buffer, num_bytes_read, client_IP, client_port);

        num_bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    }

    if (num_bytes_read == 0)
    {
        std::clog << "Client disconnected: " << client_IP << ":" << std::to_string(client_port) << std::endl;
    }
    else
    {
        std::cerr << "Failed to receive data: " << strerror(errno) << std::endl;
    }

    close(client_fd);
}

bool TCPServer::checkRateLimit(const std::string &ip)
{

    std::lock_guard<std::mutex> lock(rate_mutex);
    
    int time_interval_in_sec = 10;
    int max_req_per_interval_per_usr = 2;
    auto &count = std::get<0>(client_rate_limit[ip]);
    auto &last_time = std::get<1>(client_rate_limit[ip]);

    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - last_time).count() < time_interval_in_sec)
    {
        if (++count > max_req_per_interval_per_usr)
        {
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
