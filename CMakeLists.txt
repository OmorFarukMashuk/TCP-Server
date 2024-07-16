cmake_minimum_required(VERSION 3.10)

# Set the project name and version
project(TCP-SERVER VERSION 1.0)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add executable targets for the server and client
add_executable(server main.cpp server.cpp)
add_executable(client client.cpp)

# If additional threads library is needed (for std::thread, etc.)
find_package(Threads REQUIRED)
target_link_libraries(server PRIVATE Threads::Threads)