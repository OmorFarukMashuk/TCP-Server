# README #

### What is this repository for? ###
TBD
### How do I get Set up? ###

* Prerequisites
  * CMake or,
  * g++ compiler

* g++ Compiler
  * Open terminal on project directory 
  * For server.cpp  run -  
```g++ -std=c++11 -o server server.cpp -pthread && ./server <port_number_you_want_to_run_on>```
  *  For client.cpp  run -  
```g++ -std=c++11 -o client client.cpp && ./client 127.0.0.1 <server_running_port_number>```
* CMake
  * Open terminal on project directory 
  * For server.cpp  run - 
```mkdir build && cd build && cmake .. && make && ./server <port_number_you_want_to_run_on>``` 
  * For client.cpp run -
```./client 127.0.0.1 <server_running_port_number>```