cmake_minimum_required(VERSION 3.0)


project(FTPServer)

set(SERVER_FILES server.c)

set(SERVER_LIB server_lib.c)

add_library(server_lib STATIC ${SERVER_LIB})

add_executable(server ${SERVER_FILES})

# Указываем, что нужно линковаться с библиотекой Ws2_32
target_link_libraries(server Ws2_32 server_lib)
target_link_libraries(server_lib Ws2_32)