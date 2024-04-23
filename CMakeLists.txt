cmake_minimum_required(VERSION 3.28)
project(FTPServer)

set(CMAKE_CXX_STANDARD 17)

add_executable(FTPServer server_run/server.cpp
        server_core/source/ServerCore.cpp
        server_core/include/ServerCore.h
        server_core/include/general.h
        ftp_specification/FTPSpecification.cpp
        ftp_specification/FTPSpecification.h
        json_reader/source/json_reader.cpp
        json_reader/include/json_reader.h
)

add_executable(FTPClient client_run/client.cpp
        json_reader/source/json_reader.cpp
        json_reader/include/json_reader.h
)
