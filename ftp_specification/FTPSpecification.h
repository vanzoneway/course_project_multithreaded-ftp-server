#pragma once

#include <cstring>
#include <sys/ioctl.h>
#include <iostream>
#include <sys/socket.h>
#include <dirent.h>
#include <sys/stat.h>
#include <csignal>


#define LIST_COMMAND "LIST"
#define CWD_COMMAND "CWD"
#define DOWNLOAD_COMMAND "RETR"
#define ECHO_COMMAND "ECHO"

#define SUCCESSFULLY_CONNECTED "\033[1;32m220: Welcome to FTP Server\033[0m" // green
#define BAD_SEQUENCE_OF_COMMANDS "\033[1;31m503: Bad sequence of commands.\033[0m" // red
#define INVALID_USERNAME_OR_PASSWORD "\033[1;31m430: Invalid username or password\033[0m" // red
#define USERNAME_ACCEPTED "\033[1;32m331: User name okay, need password.\033[0m" // green
#define PASSWORD_ACCEPTED "\033[1;32m230: User logged in, proceed. Logged out if appropriate.\033[0m" // green
#define SUCCESSFUL_QUIT "\033[1;32m221: Successful Quit.\033[0m" // green
#define SYNTAX_ERROR "\033[1;31m501: Syntax error in parameters or arguments.\033[0m" // red
#define INVALID_PATH "\033[1;31m404: No such directory\033[0m" // red
#define NEED_FOR_ACCOUNT "\033[1;31m332: Need account for login.\033[0m" // red
#define INTERNAL_SERVER_ERROR "\033[1;31m500: Error\033[0m" // red
#define LIST_TRANSFER_DONE "\033[1;32m226: List transfer done.\033[0m" // green
#define SUCCESSFUL_CHANGE "\033[1;32m250: Successful change.\033[0m" // green
#define SUCCESSFUL_DOWNLOAD "\033[1;32m226: Successful download.\033[0m" // green
#define FILE_UNAVAILABLE "\033[1;31m550: File unavailable.\033[0m" // red
#define ERROR_SENDING_FILE "\033[1;31m451: Error sending file.\033[0m" // red
#define DONE_SUCCESSFULLY "\033[1;32m200: Ok\033[0m" // green

#define BUFFER_SIZE 1024


/**

    @class FTPSpecification
    @brief Handles FTP commands and communication between the server and client.
    The FTPSpecification class processes FTP commands received from the client. All command-specific handler
    functions are utilized within the handler function, which selects the appropriate handler based on the command
    received from the client.
    The server creates two sockets within the client: command_socket is used for sending commands to the server, and
    the server returns response codes and corresponding messages via the same channel. The response code, typically
    represented by the initial digits of the response, should be processed by the client.
    On the other hand, data_socket is used for transmitting data from the client to the server. For example, in the
    case of the RETR command, the data following the command (e.g., "file.txt") is sent via the data socket. From the server's perspective, all data is sent to the client via the data socket.
    When working with FTPSpecification.cpp, pay attention to the order of commands sent from the server to the client,
    as the order of commands impacts how the client should handle them. Typically, data is first read from the command
    socket and data socket on the server side, processed, and then response codes are sent back to the client. This
    process may occur multiple times, so ensure that you handle the sequencing correctly when implementing the client.
    @note This class encapsulates the FTP command processing and handles the communication between the server and client
    according to the FTP protocol specifications.
    */
class FTPSpecification {

private:
     std::string current_dir = ".";

public:
    void handler(char command[], int fcs, int fds);

private:
    void echo_handler(int fcs, int fds);
    void list_handler(int fcs, int fds);
    void cwd_handler(int fcs, int fds);
    void retr_handler(int fcs, int fds);

    void clear_socket_data(int socket_fd);
    std::string parse_current_dir();
    std::string get_client_info(int fcs);

};


