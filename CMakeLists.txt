cmake_minimum_required(VERSION 3.0)
project(SimpleServer)

set(SERVER_FILES server.c)

add_executable(server ${SERVER_FILES})

# Указываем, что нужно линковаться с библиотекой Ws2_32
target_link_libraries(server Ws2_32)