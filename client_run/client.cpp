#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>

#include "../json_reader/include/json_reader.h"

#define PATH_TO_JSON "../server_core/resources/config.json"
#define PATH_TO_DOWNLOADS "../client_downloads"
#define BUFFER_SIZE 1024

int get_code_status(const char* buffer) {
    char buff[3];
    for(int i = 7; i < 10; i++) {
        buff[i - 7] = buffer[i];
    }
    return std::stoi(buff);
}

void send_to_command_socket(const int& command_socket, const char* command, bool& is_login, bool& is_password)
{
    if (send(command_socket, command, strlen(command), 0) == -1) {
        if (errno == EPIPE) {
            std::cout << "Connection with server_core is interrupted" << std::endl;
            is_login = false;
            is_password = false;
        } else {
            std::cerr << "Error of sending data: " << strerror(errno) << std::endl;
            is_login = false;
            is_password = false;
        }
    }
}
void send_to_command_socket(const int& command_socket, const char* command, bool& stop_flag)
{
    if (send(command_socket, command, strlen(command), 0) == -1) {
        if (errno == EPIPE) {
            std::cout << "Connection with server_core is interrupted" << std::endl;
            stop_flag = true;
        } else {
            std::cerr << "Error of sending data: " << strerror(errno) << std::endl;
            stop_flag = true;
        }
    }
}
void send_to_data_socket(const int& data_socket, const char* buff, bool& stop_flag)
{
    if (send(data_socket, buff, strlen(buff), 0) == -1) {
        if (errno == EPIPE) {
            std::cout << "Connection with server_core is interrupted" << std::endl;
            stop_flag = true;
        } else {
            std::cerr << "Error of sending data: " << strerror(errno) << std::endl;
            stop_flag = true;
        }
    }
}
void send_to_data_socket(const int& data_socket, const char* buff, bool& is_login, bool& is_password)
{
    if (send(data_socket, buff, strlen(buff), 0) == -1) {
        if (errno == EPIPE) {
            std::cout << "Connection with server_core is interrupted" << std::endl;
            is_login = false;
            is_password = false;
        } else {
            std::cerr << "Error of sending data: " << strerror(errno) << std::endl;
            is_login = false;
            is_password = false;
        }
    }
}

void check_bytes_received(ssize_t bytes_received, bool& stop_flag) {
    if (bytes_received == 0) {
        std::cout << "Connection with server_core is interrupted" << std::endl;
        stop_flag = true;
    } else if (bytes_received == -1) {
        std::cerr << "Error of getting data" << strerror(errno) << std::endl;
        stop_flag = true;
    }
}
void check_bytes_received(ssize_t bytes_received, bool& is_login, bool& is_password) {
    if (bytes_received == 0) {
        std::cout << "Connection with server_core is interrupted" << std::endl;
        is_login = false;
        is_password = false;
    } else if (bytes_received == -1) {
        std::cerr << "Error of getting data" << strerror(errno) << std::endl;
        is_login = false;
        is_password = false;
    }
}



void echo_command(const char* command, char* buff,  const int& command_socket, const int& data_socket, bool& stop_flag) {

    send_to_command_socket(command_socket, command, stop_flag);
    send_to_data_socket(data_socket, buff, stop_flag);

    ssize_t bytes_received = recv(command_socket, buff, BUFFER_SIZE, 0);
    buff[bytes_received] = '\0';

    if (bytes_received == 0) {
        std::cout << "Connection with server_core is interrupted" << std::endl;
        stop_flag = true;
    } else if (bytes_received == -1) {
        std::cerr << "Error of getting data" << strerror(errno) << std::endl;
        stop_flag = true;
    }

    std::cout << buff << std::endl;

    if(get_code_status(buff) == 501 || get_code_status(buff) == 503)
        return;

    if(get_code_status(buff) == 221){
        stop_flag = true;
        return;
    }

    bytes_received = recv(data_socket, buff, BUFFER_SIZE, 0);
    buff[bytes_received] = '\0';

    if (bytes_received == 0) {
        std::cout << "Connection with server_core is interrupted" << std::endl;
        stop_flag = true;
    } else if (bytes_received == -1) {
        std::cerr << "Error of getting data" << strerror(errno) << std::endl;
        stop_flag = true;
    }

    std::cout << buff << std::endl;

}

void list_command(const char* command, char* buff, const int& command_socket, const int& data_socket, bool& stop_flag) {

    send_to_command_socket(command_socket, command, stop_flag);

    ssize_t bytes_received = recv(command_socket, buff, BUFFER_SIZE, 0);
    buff[bytes_received] = '\0';

    check_bytes_received(bytes_received, stop_flag);

    std::cout << buff << std::endl;

    int status_code = get_code_status(buff);

    if(status_code == 501 || status_code == 503 || status_code == 500)
        return;

    if(status_code == 221){
        stop_flag = true;
        return;
    }

    int flags = fcntl(data_socket, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "Error of getting sockets flags" << std::endl;
        return;
    }
    if (fcntl(data_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "Error of setting sockets flags" << std::endl;
        return;
    }

    std::string result;
    char buffer[1024];
    bytes_received = 0;

    while (true) {
        ssize_t received = recv(data_socket, buffer, sizeof(buffer), 0);
        if (received == -1 || received == 0)
            break;
        result.append(buffer, received);
        bytes_received += received;
    }


    flags &= ~O_NONBLOCK;
    if (fcntl(data_socket, F_SETFL, flags) == -1) {
        std::cerr << "Error of setting sockets flags" << std::endl;
        return;
    }

    std::cout << result << std::endl;

}

void cwd_command(const char* command, char* buff, const int& command_socket, const int& data_socket, bool& stop_flag) {
    send_to_command_socket(command_socket, command, stop_flag);
    send_to_data_socket(data_socket, buff, stop_flag);

    ssize_t bytes_received = recv(command_socket, buff, BUFFER_SIZE, 0);
    buff[bytes_received] = '\0';

    check_bytes_received(bytes_received, stop_flag);

    std::cout << buff << std::endl;

    int status_code = get_code_status(buff);

    if(status_code == 501 || status_code == 503 || status_code == 500 || status_code == 404)
        return;

    if(status_code == 221){
        stop_flag = true;
        return;
    }

    bytes_received = recv(data_socket, buff, BUFFER_SIZE, 0);
    buff[bytes_received] = '\0';

    check_bytes_received(bytes_received, stop_flag);

    std::cout << buff << std::endl;

}

void authorize(int fcs, int fds) {
    bool is_login = false;
    bool is_password = false;


    while (!is_login) {
        char command[BUFFER_SIZE];
        char data[BUFFER_SIZE];

        std::cin >> command;
        if (std::cin.peek() == ' ') {
            std::cin.ignore();
            std::cin.getline(data, sizeof(data));
        } else {
            data[0] = '\0';
        }

        send_to_command_socket(fcs, command, is_login, is_password);
        send_to_data_socket(fds, data, is_login, is_password);

        ssize_t bytes_received = recv(fcs, command, BUFFER_SIZE, 0);
        command[bytes_received] = '\0';

        check_bytes_received(bytes_received, is_login, is_password);

        std::cout << command << std::endl;

        int status_code = get_code_status(command);
        if (status_code == 331)
            is_login = true;

        memset(command, 0, BUFFER_SIZE);
        memset(command, 0, BUFFER_SIZE);

    }

    while (!is_password) {
        char command[BUFFER_SIZE];
        char data[BUFFER_SIZE];

        std::cin >> command;
        if (std::cin.peek() == ' ') {
            std::cin.ignore();
            std::cin.getline(data, sizeof(data));
        } else {
            data[0] = '\0';
        }

        send_to_command_socket(fcs, command, is_login, is_password);
        send_to_data_socket(fds, data, is_login, is_password);

        ssize_t bytes_received = recv(fcs, command, BUFFER_SIZE, 0);
        command[bytes_received] = '\0';

        check_bytes_received(bytes_received, is_login, is_password);

        std::cout << command << std::endl;

        int status_code = get_code_status(command);
        if (status_code == 230)
            is_password = true;

        memset(command, 0, BUFFER_SIZE);
        memset(command, 0, BUFFER_SIZE);

    }
}

void quit_command(const char* command, char* buff, const int& command_socket, const int& data_socket, bool& stop_flag) {
    send_to_command_socket(command_socket, command, stop_flag);
    ssize_t bytes_received = recv(command_socket, buff, BUFFER_SIZE, 0);
    buff[bytes_received] = '\0';

    check_bytes_received(bytes_received, stop_flag);

    std::cout << buff << std::endl;
    stop_flag = true;
}

void retr_command(const char* command, char* buff, const int& command_socket, const int& data_socket, bool& stop_flag) {
    int status_code;
    int size_of_file_from_server;
    std::string path_to_file = std::string(PATH_TO_DOWNLOADS) + "/" + buff;

    timeval tv_recv{};
    tv_recv.tv_sec = 3;
    tv_recv.tv_usec = 0;
    setsockopt(data_socket, SOL_SOCKET, SO_RCVTIMEO, &tv_recv, sizeof(tv_recv));

    send_to_command_socket(command_socket, command, stop_flag);
    send_to_data_socket(data_socket, buff, stop_flag);

    std::ofstream file (path_to_file, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing." << std::endl;
    }
    //проверка, есть ли такой файл в текущей директории и получилось ли его открыть.(сервер возращает 200 в случае успеха )
    ssize_t bytes_received = recv(command_socket, buff, BUFFER_SIZE, 0);
    buff[bytes_received] = '\0';
    check_bytes_received(bytes_received, stop_flag);

    status_code = get_code_status(buff);
    if(status_code == 550 || status_code == 503 || status_code == 501) {
        std::cout << buff << std::endl;
        file.close();
        return;
    }


    std::string result;
    char buffer[1024];
    bytes_received = 0;
    recv(data_socket, &size_of_file_from_server, sizeof(int), 0);
    while (true) {
        ssize_t received = recv(data_socket, buffer, sizeof(buffer), 0);
        if (received == -1 || received == 0)
            break;
        file.write(buffer, received);
        bytes_received += received;
    }

    if(bytes_received != size_of_file_from_server) {
        std::cout << "Doesnt similar sizes of files on server and on client" << std::endl;
    }

    bytes_received = recv(command_socket, buff, BUFFER_SIZE, 0);
    buff[bytes_received] = '\0';
    check_bytes_received(bytes_received, stop_flag);
    status_code = get_code_status(buff);
    //проверка на успех передачи со стороны сервера ( возращает 226 в случае успеха )

    if(status_code != 226) {

        std::cout << buff << std::endl;
        file.close();
        std::remove(path_to_file.c_str());
        return;
    }

    std::cout << buff << std::endl;

    tv_recv.tv_sec = 0;
    tv_recv.tv_usec = 0;
    setsockopt(data_socket, SOL_SOCKET, SO_RCVTIMEO, &tv_recv, sizeof(tv_recv));
    file.close();

}

int main() {

    std::string json = Json_Reader::get_json(PATH_TO_JSON);
    int server_port = stoi(Json_Reader::find_value(json, "serverPort"));
    std::string local_ip_address = Json_Reader::find_value(json, "localIpAddress");

    int command_socket = socket(AF_INET, SOCK_STREAM, 0);
    int data_socket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr(local_ip_address.data());
    if(connect(command_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 ) {
        std::cout << "cannot to connect to host" << std::endl;
    }

    server_addr.sin_port = htons(server_port);
    if(connect(data_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 ) {
        std::cout << "cannot to connect to host" << std::endl;
    }

    char buff[BUFFER_SIZE];
    recv(command_socket, buff, sizeof(buff), 0);
    std::cout << buff << std::endl;

    authorize(command_socket, data_socket);

    bool stop_flag = false;
    while (!stop_flag) {
        char command[BUFFER_SIZE];
        char data[BUFFER_SIZE];

        std::cin >> command;
        if (std::cin.peek() == ' ') {
            std::cin.ignore();
            std::cin.getline(data, sizeof(data));
        } else {
            data[0] = '\0';
        }

        if(strcmp(command, "ECHO") == 0) {
            echo_command(command, data, command_socket, data_socket, stop_flag);
        }else if (strcmp(command, "LIST") == 0){
            list_command(command, data, command_socket, data_socket, stop_flag);
        }else if (strcmp(command, "CWD") == 0){
            cwd_command(command, data, command_socket, data_socket, stop_flag);
        }else if(strcmp(command, "RETR") == 0){
            retr_command(command, data, command_socket, data_socket, stop_flag);
        }else if (strcmp(command, "QUIT") == 0){
            quit_command(command, data, command_socket, data_socket, stop_flag);
            break;
        }


        memset(command, 0, BUFFER_SIZE);
        memset(command, 0, BUFFER_SIZE);

    }

    close(data_socket);
    close(command_socket);

    return 0;


}