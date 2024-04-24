#include "../include/ServerCore.h"

///NOTE IF YOU WANT TO CHANGE DIRECTORY AND AFTER THAT CONNECT FROM ANOTHER CLIENT
///YOU HAVE TO GO BACK TO HOME DIRECTORY BECAUSE JSON CHDIR CHANGES DIRECTORY
///AND MAKE CWD COMMAND PROPERLY ( use path in class )

/**

    @brief Starts the server application.
    The start function first calls the create_bind_listen_sockets method to set up the sockets in the required state.
    Then, it invokes the thread_pool, where the handlingAccept function is asynchronously executed in the background thread.
    The handlingAccept function contains an infinite loop for accepting new clients.
    Once a client is accepted, another thread is spawned from the same thread pool to handle the server-side client operations,
    including the authentication process through the handle_command function.
    @note This function should be called to initiate the server application.
    */
void ServerCore::start() {

    create_bind_listen_sockets();

}

void ServerCore::create_bind_listen_sockets() {

    std::string json = Json_Reader::get_json(PATH_TO_JSON);
    server_port = std::stoi(Json_Reader::find_value(json, "serverPort"));
    local_ip_address = Json_Reader::find_value(json, "localIpAddress");
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        std::cerr << "Error of creating a server_socket" << std::endl;
        return;
    }

    sockaddr_in server_hint{};
    server_hint.sin_family = AF_INET;
    server_hint.sin_port = htons(server_port);
    server_hint.sin_addr.s_addr = inet_addr(local_ip_address.data());



    if (bind(server_socket, reinterpret_cast<struct sockaddr*>(&server_hint), sizeof(server_hint)) < 0) {
        std::cout << "Failed to bind server_socket." << std::endl;
        return;
    }


    if(listen(server_socket, SOMAXCONN) == -1) {
        std::cout << "Failed to listen data_socket" << std::endl;
        return;
    }

    thread_pool.addJob([this]{handlingAccept();});


}


/**

    @brief Function for accepting new clients.
    The handlingAccept function is executed in a background thread to continuously accept new client connections.
    Upon accepting a client, a new thread is created to handle server-side client operations,
    such as the authentication process through the handle_command function.
    This function runs indefinitely until the server is stopped.
    */
void ServerCore::handlingAccept() {

    while (true) {

        auto* new_client = new ServerClient;

        sockaddr_in client_addr{};

        socklen_t addrlen = sizeof(client_addr);

        new_client->command_socket = accept(server_socket, (struct sockaddr *) &client_addr, &addrlen);

        new_client->data_socket = accept(server_socket, (struct sockaddr *) &client_addr, &addrlen);

        new_client->connected();

        /**

        @brief Handles the command received from the client.
        The handle_command function is responsible for processing commands received from the client.
        It performs the necessary operations, including authentication, based on the received command.
        @param client The client connection object.
        */
        std::thread tr([new_client](){
            char buffer[1024];
            ssize_t valread;

            new_client->authorize();
            while(true) {

                valread = new_client->get_command_from_client(buffer);
                if (strcmp(buffer, "QUIT") == 0 || valread == -1 || valread == 0)
                {
                    if(new_client->is_authorized) {
                        send(new_client->command_socket, SUCCESSFUL_QUIT, strlen(SUCCESSFUL_QUIT), 0);
                        new_client->disconnect();
                    }

                    break;
                }
                else
                    new_client->handle_command(buffer);

            }

        });
        tr.detach();

    }

}

void ServerCore::joinLoop() {
    thread_pool.join();
}

void ServerClient::disconnect() {

    sockaddr_in address {};
    int addrlen = sizeof(address);
    getpeername(command_socket, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    std::cout << "\033[1;31mGuest disconnected, ip\033[0m " << inet_ntoa(address.sin_addr)
              << " , \033[1;31mport\033[0m " << ntohs(address.sin_port) << std::endl;
    close(data_socket);
    close(command_socket);
    command_socket = 0;
    data_socket = 0;

}

void ServerClient::connected() const {
    sockaddr_in address {};
    int addrlen = sizeof(address);
    getpeername(command_socket, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    std::cout << "\033[1;32mGuest connected, ip\033[0m " << inet_ntoa(address.sin_addr)
              << " , \033[1;32mport\033[0m " << ntohs(address.sin_port) << std::endl;
    send(command_socket, SUCCESSFULLY_CONNECTED, strlen(SUCCESSFULLY_CONNECTED), 0);
}

ssize_t ServerClient::get_command_from_client(char buffer[]) const {
    ssize_t valread;
    valread = recv(command_socket, buffer, PACKET_SIZE, 0);
    buffer[valread] = '\0';
    return valread;
}

void ServerClient::handle_command(char command[]) const {
    ftp_specification->handler(command, command_socket, data_socket);
}

void ServerClient::authorize() {

    char buffer[1024];
    size_t valread;
    bool is_login = false;
    bool is_password = false;
    std::string json = Json_Reader::get_json(PATH_TO_JSON);
    std::string login_name;




    while(!is_login) {
        memset(buffer, 0, sizeof(buffer));
        valread = get_command_from_client(buffer);
        if (strcmp(buffer, "QUIT") == 0 || valread == -1 || valread == 0)
        {
            send(command_socket, SUCCESSFUL_QUIT, strlen(SUCCESSFUL_QUIT), 0);
            disconnect();
            free(this);
            is_password = true;
            break;
        }
        else if (strcmp(buffer, "USER") == 0)
        {

            valread = get_data_from_client(buffer);
            if(valread == -1 || valread == 0) {
                send(command_socket, INVALID_USERNAME_OR_PASSWORD, strlen(INVALID_USERNAME_OR_PASSWORD), 0);
                clear_socket_data(data_socket);
                continue;
            }
            std::vector<std::string> json_vector = Json_Reader::split_array(Json_Reader::find_value(json, "users"));
            std::string name;
            for (const auto &user_info: json_vector) {
                name = Json_Reader::find_value(user_info, "user");
                if (strcmp(name.c_str(), buffer) == 0) {
                    login_name = name.c_str();
                    is_login = true;
                }
            }
            if (is_login) {
                send(command_socket, USERNAME_ACCEPTED, strlen(USERNAME_ACCEPTED), 0);
                clear_socket_data(data_socket);
            } else {
                send(command_socket, INVALID_USERNAME_OR_PASSWORD, strlen(INVALID_USERNAME_OR_PASSWORD), 0);
                clear_socket_data(data_socket);

            }

        } else {
            send(command_socket, NEED_FOR_ACCOUNT, strlen(NEED_FOR_ACCOUNT), 0);
            clear_socket_data(data_socket);
        }
    }

    while(!is_password) {
        memset(buffer, 0, sizeof(buffer));
        valread = get_command_from_client(buffer);
        if (strcmp(buffer, "QUIT") == 0 || valread == -1 || valread == 0)
        {
            send(command_socket, SUCCESSFUL_QUIT, strlen(SUCCESSFUL_QUIT), 0);
            disconnect();
            free(this);
            break;
        }
        else if (strcmp(buffer, "PASS") == 0)
        {
            valread = get_data_from_client(buffer);
            if(valread == -1 || valread == 0) {
                send(command_socket, INVALID_USERNAME_OR_PASSWORD, strlen(INVALID_USERNAME_OR_PASSWORD), 0);
                clear_socket_data(data_socket);
                continue;
            }
            std::vector<std::string> json_vector = Json_Reader::split_array(Json_Reader::find_value(json, "users"));
            std::string password;
            std::string name;
            for (const auto &user_info: json_vector) {
                password = Json_Reader::find_value(user_info, "password");
                name = Json_Reader::find_value(user_info, "user");
                if (strcmp(password.c_str(), buffer) == 0 && strcmp(login_name.c_str(), name.c_str()) == 0) {
                    is_password = true;
                }
            }
            if (is_password) {
                send(command_socket, PASSWORD_ACCEPTED, strlen(PASSWORD_ACCEPTED), 0);
                clear_socket_data(data_socket);
            } else {
                send(command_socket, INVALID_USERNAME_OR_PASSWORD, strlen(INVALID_USERNAME_OR_PASSWORD), 0);
                clear_socket_data(data_socket);
            }


        } else {
            send(command_socket, NEED_FOR_ACCOUNT, strlen(NEED_FOR_ACCOUNT), 0);
            clear_socket_data(data_socket);
        }
    }

    if(is_password && is_login) {
        std::cout << "\033[1;32mAuthorized successfully\033[0m" << std::endl;
        is_authorized = true;
    }

}

size_t ServerClient::get_data_from_client(char *buffer) {

    timeval tv_recv{};
    tv_recv.tv_sec = 1;
    tv_recv.tv_usec = 0;
    setsockopt(data_socket, SOL_SOCKET, SO_RCVTIMEO, &tv_recv, sizeof(tv_recv));

    size_t valread;
    valread = recv(data_socket, buffer, PACKET_SIZE, 0);
    buffer[valread] = '\0';

    tv_recv.tv_sec = 0;
    tv_recv.tv_usec = 0;
    setsockopt(data_socket, SOL_SOCKET, SO_RCVTIMEO, &tv_recv, sizeof(tv_recv));

    return valread;
}

void ServerClient::clear_socket_data(int socket_fd) {
    int bytes_available;
    ioctl(socket_fd, FIONREAD, &bytes_available);
    char buffer[bytes_available];
    if(bytes_available > 0) {
        recv(socket_fd, buffer, bytes_available, 0);
    }
}
