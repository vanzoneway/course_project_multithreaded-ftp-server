#include "FTPSpecification.h"

std::mutex FTPSpecification::retr_mutex;

void FTPSpecification::handler(char* command , int fcs, int fds) {

    if(strcmp(command, ECHO_COMMAND) == 0) {
        echo_handler(fcs, fds);
    }else if(strcmp(command, LIST_COMMAND) == 0) {
        list_handler(fcs, fds);
    }else if(strcmp(command, CWD_COMMAND) == 0) {
        cwd_handler(fcs, fds);
    }else if(strcmp(command, DOWNLOAD_COMMAND) == 0) {
        {
            std::lock_guard<std::mutex> lock(retr_mutex);
            retr_handler(fcs, fds);
        }
    }else
    {
        send(fcs, BAD_SEQUENCE_OF_COMMANDS, strlen(BAD_SEQUENCE_OF_COMMANDS), 0);
        clear_socket_data(fds);
    }

}

void FTPSpecification::echo_handler(int fcs, int fds) {

    char buff[1024];
    ssize_t valread;

    timeval tv_recv{};
    tv_recv.tv_sec = 1;
    tv_recv.tv_usec = 0;
    setsockopt(fds, SOL_SOCKET, SO_RCVTIMEO, &tv_recv, sizeof(tv_recv));

    valread = recv(fds, buff, sizeof(buff), 0);
    if(valread == -1 || valread == 0) {
        send(fcs, SYNTAX_ERROR, strlen(SYNTAX_ERROR), 0);
        return;
    }

    tv_recv.tv_sec = 0;
    tv_recv.tv_usec = 0;
    setsockopt(fds, SOL_SOCKET, SO_RCVTIMEO, &tv_recv, sizeof(tv_recv));

    buff[valread] = '\0';
    std::cout << "\033[1;34mECHO command:\033[0m " << buff << get_client_info(fcs) << std::endl;
    send(fds, buff, strlen(buff), 0);
    send(fcs, DONE_SUCCESSFULLY, strlen(DONE_SUCCESSFULLY), 0);

}

void FTPSpecification::clear_socket_data(int socket_fd) {
    int bytes_available;
    ioctl(socket_fd, FIONREAD, &bytes_available);
    char buffer[bytes_available];
    if(bytes_available > 0) {
        recv(socket_fd, buffer, bytes_available, 0);
    }
}

void FTPSpecification::list_handler(int fcs, int fds) {

    std::string result = parse_current_dir();
    if(strcmp(result.data(), INTERNAL_SERVER_ERROR) == 0) {
        send(fcs, INTERNAL_SERVER_ERROR, strlen(INTERNAL_SERVER_ERROR), 0);
        return;
    }
    ssize_t bytes_sent = 0;
    const char* data = result.c_str();
    while(bytes_sent < result.length())
    {
        int bytes_to_send = std::min(1024, (int)result.length() - (int)bytes_sent);
        ssize_t sent = send(fds, data + bytes_sent, bytes_to_send, 0);
        if (sent == -1) {
            send(fcs, INTERNAL_SERVER_ERROR, strlen(INTERNAL_SERVER_ERROR), 0);
            return;
        }
        bytes_sent += sent;
    }
    send(fcs, LIST_TRANSFER_DONE, strlen(LIST_TRANSFER_DONE), 0);
    std::cout << "\033[1;34mLIST command:\033[0m " << get_client_info(fcs) << std::endl;


}

std::string FTPSpecification::parse_current_dir() {
    DIR* dir = opendir(current_dir.c_str());
    std::string result;
    if (dir == nullptr) {
        std::cerr << "Error of opening dir: " << current_dir << std::endl;
        return INTERNAL_SERVER_ERROR;
    }
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_REG || entry->d_type == DT_DIR) {
            if (entry->d_name[0] != '.') {
                if (entry->d_type == DT_DIR) {
                    result.append("\033[1;34m"); // Blue colour
                } else {
                    result.append("\033[1;35m"); // Pink colour
                }
                result.append(entry->d_name);
                result.append("\033[0m"); // Reset colour
                result.append("\n");
            }
        }
    }

    closedir(dir);
    return result;
}

void FTPSpecification::cwd_handler(int fcs, int fds) {
    char buff[1024];
    ssize_t valread;
    std::string old_current_dir = current_dir;

    int bytes_available;
    ioctl(fds, FIONREAD, &bytes_available);
    if(bytes_available == 0) {
        send(fcs, SYNTAX_ERROR, strlen(SYNTAX_ERROR), 0);
        return;
    }
    valread = recv(fds, buff, sizeof(buff), 0);
    buff[valread] = '\0';
    std::cout << "\033[1;34mCWD command:\033[0m " << buff << get_client_info(fcs) << std::endl;


    if (chdir(old_current_dir.c_str()) == -1) {
        send(fcs, INVALID_PATH, strlen(INVALID_PATH), 0);
        return;
    }

    struct stat statbuf{};
    if (stat(buff, &statbuf) == -1) {
        send(fcs, INVALID_PATH, strlen(INVALID_PATH), 0);
        return;
    }

    if (chdir(buff) == -1) {
        send(fcs, INVALID_PATH, strlen(INVALID_PATH), 0);
        return;
    }
    current_dir = std::filesystem::current_path();

    if (chdir(baser_dir.c_str()) == -1) {
        send(fcs, INVALID_PATH, strlen(INVALID_PATH), 0);
        return;
    }


    send(fds, current_dir.c_str(), strlen(current_dir.c_str()), 0);
    send(fcs, SUCCESSFUL_CHANGE, strlen(SUCCESSFUL_CHANGE), 0);
}

void FTPSpecification::retr_handler(int fcs, int fds) {
    char buff[BUFFER_SIZE];
    ssize_t valread;
    int size_of_file;
    std::string path_to_file;

    valread = recv(fds, buff, sizeof(buff), 0);
    buff[valread] = '\0';
    path_to_file = current_dir + "/" + buff;
    std::cout << "\033[1;34mRETR command:\033[0m " << buff << get_client_info(fcs) << std::endl;

    if (!std::filesystem::exists(path_to_file)) {
        send(fcs, FILE_UNAVAILABLE, strlen(FILE_UNAVAILABLE), 0);
        return;
    }
    if (!std::filesystem::is_regular_file(path_to_file)) {
        send(fcs, FILE_UNAVAILABLE, strlen(FILE_UNAVAILABLE), 0);
        return;
    }
    
    std::ifstream file(path_to_file, std::ios::binary);

    if(!file.is_open()) {
        send(fcs, FILE_UNAVAILABLE, strlen(FILE_UNAVAILABLE), 0);
        return;
    }

    size_of_file = std::filesystem::file_size(path_to_file);

    send(fcs,DONE_SUCCESSFULLY, strlen(DONE_SUCCESSFULLY), 0);

    send(fds, &size_of_file, sizeof(int), 0);

    char buffer_to_send[1024];
    ssize_t total_bytes_sent = 0;
    while (true) {

        file.read(buffer_to_send, sizeof(buffer_to_send));
        ssize_t bytes_read = file.gcount();
        if (bytes_read == 0) {
            break;
        } else if (bytes_read == -1) {
            send(fcs, ERROR_SENDING_FILE, strlen(ERROR_SENDING_FILE), 0);
            file.close();
            return;
        }
        ssize_t bytes_sent = send(fds, buffer_to_send, bytes_read, 0);
        total_bytes_sent += bytes_sent;
        if (bytes_sent == -1) {
            send(fcs, ERROR_SENDING_FILE, strlen(ERROR_SENDING_FILE), 0);
            file.close();
            return;
        }
    }

    if(total_bytes_sent != size_of_file) {
        send(fcs, ERROR_SENDING_FILE, strlen(ERROR_SENDING_FILE), 0);
    } else {
        send(fcs, SUCCESSFUL_DOWNLOAD, strlen(SUCCESSFUL_DOWNLOAD), 0);
    }

}

std::string FTPSpecification::get_client_info(int fcs) {
    sockaddr_in address {};
    int addrlen = sizeof(address);
    getpeername(fcs, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    std::string result;
    result.append("\033[1;34m IP:\033[0m "); // Синий цвет для IP
    result.append("\033[1;32m"); // Зеленый цвет для адреса
    result.append(inet_ntoa(address.sin_addr));
    result.append("\033[0m, ");
    result.append("\033[1;34mPORT:\033[0m "); // Синий цвет для PORT
    result.append("\033[1;32m"); // Зеленый цвет для порта
    result.append(std::to_string(ntohs(address.sin_port)));
    result.append("\033[0m");
    return result;
}

std::vector<std::string> FTPSpecification::split_path(const std::string &path_string) {
    std::vector<std::string> commands;
    std::istringstream iss(path_string);
    std::string token;

    while (std::getline(iss, token, '/')) {
        if (!token.empty()) {
            commands.push_back(token);
        }
    }

    return commands;
}


