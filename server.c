#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>

#define PACKET_MAX_SIZE 512

char* getSockIp(SOCKET s){
    struct sockaddr_in name;
    int lenn = sizeof(name);
    ZeroMemory (&name, sizeof (name));
    if (SOCKET_ERROR == getsockname(s, (struct sockaddr*)&name, &lenn)) {
        // Error
    }
    return inet_ntoa((struct in_addr)name.sin_addr);
}

SOCKET getHost(char* ipAddr, int port) {
    SOCKET s;
    // Создание слушающего сокета
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Failed to create listening socket. Error %d\n", WSAGetLastError());
        exit(0);
    }

    struct sockaddr_in serverAddress;
    // Настройка адреса сервера
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(ipAddr);
    serverAddress.sin_port = htons(port);

    // Привязка адреса сервера к слушающему сокету
    if (bind(s, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        printf("Failed to bind address to listening socket.\n");
        closesocket(s);
        exit(0);
    }

    // Прослушивание входящих соединений
    if (listen(s, SOMAXCONN) == SOCKET_ERROR) {
        printf("Error while listening for incoming connections.\n");
        closesocket(s);
        exit(0);
    }
    return s;
}

int main() {
    struct sockaddr_in clientAddress;
    char response[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>Hello, World!</h1>";

    // Инициализация Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Error %d during initialization Winsock.\n", WSAGetLastError());
        return 1;
    }

    // Создание слушающего сокета, привязка к адресу
    SOCKET listenSocket = getHost("127.0.0.2", 8080);

    while (1) {
        printf("The server is up at %s and waiting for connections...\n", getSockIp(listenSocket));

        // Принятие входящего соединения
        int clientAddressSize = sizeof(clientAddress);
        SOCKET clientSocket = accept(listenSocket, (struct sockaddr*)&clientAddress, &clientAddressSize);
        if (clientSocket == INVALID_SOCKET) {
            printf("An error occurred while accepting an incoming connection.\n");
            closesocket(listenSocket);
            return 1;
        }

        printf("New connection with %s received.\n", getSockIp(clientSocket));

        // Отправка сообщения клиенту
        send(clientSocket, "Hello Client! Welcome to ftp-server!\n\nSend \"stop\" for stop server.\n", sizeof(response) - 1, 0);

        // Получение сообщения
        int len; 
        char buff[PACKET_MAX_SIZE];
        do {
            if ((len = recv(clientSocket, (char*)&buff, PACKET_MAX_SIZE, 0)) == SOCKET_ERROR) {
                return -1;
            }
            printf("Client: ");
            for (int i = 0; i < len; i++) {
                printf ("%c", buff[i]);
            }
            printf("\n");
            if(buff[0] == 's'){
                closesocket(listenSocket);
                break;
            }
        } while (len != 0);

        printf("Msg received.\n");
        // Отправка сообщения клиенту
        send(clientSocket, response, sizeof(response) - 1, 0);

        // Закрытие сокета клиента
        closesocket(clientSocket);

        printf("Connection closed.\n");
    }

    // Закрытие слушающего сокета
    closesocket(listenSocket);

    // Освобождение ресурсов Winsock
    WSACleanup();

    return 0;
}