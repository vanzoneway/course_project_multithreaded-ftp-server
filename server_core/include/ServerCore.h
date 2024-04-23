#pragma once

#include "general.h"
#include "../../ftp_specification/FTPSpecification.h"
#include "../../json_reader/include/json_reader.h"


#include <unistd.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <semaphore>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>

/**

    @file configuration.cpp
    @brief Configuration file for the server and client applications.
    This file contains the necessary configuration settings for the server and client applications.
    The configuration file must be located in the following directory for the server and client applications to correctly retrieve data from it:
        For the server: server_core/include/ServerCore.h
        For the client: client_run/client.cpp
    If you want to change the location of the configuration file directory, you should navigate to server_core/include/ServerCore.h
    and modify the value of the PATH_TO_JSON constant.
    Required configuration settings for the client:
        serverPort: The port number of the server.
        localIpAddress: The local IP address of the client.
    Required configuration settings for the server:
        serverPort: The port number of the server.
        localIpAddress: The local IP address of the server.
    Additional configuration settings:
        users: A list of users for the server.
        */

#define PATH_TO_JSON "../server_core/resources/config.json"


#define PACKET_SIZE 1024


/**

    @class ServerClient
    @brief Represents a server-side client entity.
    The ServerClient class represents a client from the server's perspective. Each instance of this class is created within a separate thread and dynamically allocated on the heap.
    It is not a member of the ServerCore class, but rather exists independently within each thread.
    The ServerClient class contains an instance of the FTPSpecification class, which is provided to the handler function. The handler function is responsible for processing all commands sent by the client.
    For more information on the functionality of FTPSpecification, refer to the corresponding header file.
    @note This class encapsulates the server-side client behavior and facilitates command processing and communication with the client.
    */
class ServerClient {
public:
    int command_socket;
    int data_socket;
    void (*handler)(char command[], int fcs, int fds);
    bool is_authorized = false;
    FTPSpecification* ftp_specification = new FTPSpecification();

    bool operator==(const ServerClient &other) const {
        return command_socket == other.command_socket;
    }

public:
    void disconnect();
    void connected() const;
    ssize_t get_command_from_client(char buffer[]) const;
    size_t get_data_from_client(char buffer[]);
    void handle_command(char command[]) const;
    void authorize();
    void clear_socket_data(int socket_fd);

};



/**

    @class ServerCore
    @brief The core component of our server application.
    The ServerCore class represents the heart of our server. It is responsible for managing client threads and creating instances of the ServerClient class within those threads.
    Each ServerClient object is dynamically allocated on the heap and exists until the associated thread is terminated, which occurs upon client disconnection.
    @note This class encapsulates the essential functionality of the server and serves as a central component for handling client interactions.
    */
class ServerCore {
private:
    int server_socket;
    int server_port;
    std::string local_ip_address;
    ThreadPool thread_pool;
private:
    void create_bind_listen_sockets();
    void handlingAccept();
public:
    void start();
    void joinLoop();

};



