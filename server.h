// server.h
#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <iomanip>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <thread>
#include <unordered_map>
#include <map>
#include <chrono>
#include <mutex>

#define DATA_BYTE 1030  // Considering T - 2 BYTE, L - 4 BYTE, V - 1024 BYTE
#define BYTES_TO_READ 4 // Number of bytes to be displayed

class TCPServer
{
private:
    int port;
    int server_fd;
    std::unordered_map<std::string, std::pair<int, std::chrono::steady_clock::time_point>> client_rate_limit; // This map tracks the number of requests (int) and
                                                                                                              // the last time (std::chrono::steady_clock::time_point) we recorded a request for each client IP.
                                                                                                              // The std::string key represents the client's IP address

    std::mutex rate_mutex; // Ensuring the rate limit checking and updating are thread-safe,
                           // preventing race conditions when multiple threads try to update the map simultaneously

public:
    // Constructor: Initializes the server on the specified port
    TCPServer(int port);
    // Destructor: Closes the server socket
    ~TCPServer();
    // Runs the server: Accepts and handles incoming connections in a thread
    void run();

private:
    // Initializes the server: Creates and binds the server socket
    int initializeServer();
    // Handles a client connection
    void handleClient(int client_fd, sockaddr_in client_addr);
    // Parses the received data, print the output according to given format
    void parseData(char buffer[], unsigned int num_bytes_read, char client_IP[], int client_port);
    // Checks how much time has passed since the last recorded request from the specific IP
    bool checkRateLimit(const std::string &ip);
};

#endif // SERVER_H
