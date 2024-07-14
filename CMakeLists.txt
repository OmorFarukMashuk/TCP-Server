cmake_minimum_required(VERSION 3.10)

# Set the project name and version
project(TCP-SERVER VERSION 1.0)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add executable targets for the server and client
add_executable(server server.cpp)
add_executable(client client.cpp)