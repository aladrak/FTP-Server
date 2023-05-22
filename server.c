#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define SIZE_BUF 512

int procMsg(SOCKET serv, SOCKET clnt, char* buff){

    if (buff[0] == '-') {
// Остановка сервера
        if (!strncmp(buff, "-stop", 5)) {
            // char msg[] = "Server stopped...\n\nGoodbye Client! Nice to meet you again!\n\n";
            printf("Server stopped by client.\n");
            snprintf(buff, SIZE_BUF, "Server stopped... Goodbye Client! Nice to meet you again!\n");
            send(clnt, buff, strlen(buff), 0);
            closesocket(clnt);
            closesocket(serv);
            return 1;
// Вывод помощи по командам
        } else if (!strncmp(buff, "-help", 5)) {
            snprintf(buff, SIZE_BUF, "\n-stop\t\tStopped server.\n-help\t\tHelp-manual.\n-list\t\tList of files.\n-sendfile\tTransfer from client to server.\n-getfile\tTransfer from server to client.\n");
            send(clnt, buff, strlen(buff), 0);
            return 0;
// Отправка файла от клиента
        } else if (!strncmp(buff, "-sendfile", 9)) {
            int len = 0;
            printf("Start file-transfer operation from client.\n");

            snprintf(buff, SIZE_BUF, "Send to server name of file.\n");
            send(clnt, buff, strlen(buff), 0);

            printf("Waiting for filename...\n");

            // char inputbuff[SIZE_BUF];
            if ((len = recv(clnt, buff, SIZE_BUF, 0)) == SOCKET_ERROR) {
                printf("Error getting filename.\n");
                closesocket(clnt);
                return 1;
            }
            printf("Recived filename: ");

            char filename[128];
            strncpy((char*)&filename, buff, len);
            filename[len] = '\0';
            printf("%s\n", filename);

            snprintf(buff, SIZE_BUF, "Filename received. Waiting for file.\n");
            send(clnt, buff, strlen(buff), 0);
            
            FILE *file = fopen(filename, "wb");
            if (file == NULL) {
                printf("Creating file error.\n");
                fclose(file);
                closesocket(clnt);
                return 1;
            }

            while (1) {
                if ((len = recv(clnt, buff, SIZE_BUF, 0)) == SOCKET_ERROR) {
                    printf("File retrieval error.\n");
                    fclose(file);
                    closesocket(clnt);
                    return 1;
                }
                if (!strncmp(buff, "-end", 4)) {
                    break;
                }
                if (len > 0) {
                    fwrite(buff, 1, len, file);
                } 
            }

            fclose(file);
            printf("Successfully receiving and saving the file %s!\n", filename);
            snprintf(buff, SIZE_BUF, "Successfully receiving and saving the file.\n");
            send(clnt, buff, strlen(buff), 0);
            return 0;
// Отправка файла клиенту
        } else if (!strncmp(buff, "-getfile", 8)) {

        } else if (!strncmp(buff, "-list", 5)) {

        } else if (!strncmp(buff, "-login", 6)) {

        } else {
            snprintf(buff, SIZE_BUF, "No such command found. Type -help for help.\n");
            send(clnt, buff, strlen(buff), 0);
        }
    // Отправка подтверждения получения сообщения
    } else {
        snprintf(buff, SIZE_BUF, "Msg received to client.\n");
        if (send(clnt, buff, strlen(buff), 0) == SOCKET_ERROR) {
            closesocket(clnt);
            exit(0);
        }
    }
    return 0;
}

// Функция получения адреса
char* getSockIp(SOCKET s){
    struct sockaddr_in name;
    int lenn = sizeof(name);
    ZeroMemory (&name, sizeof (name));
    if (SOCKET_ERROR == getsockname(s, (struct sockaddr*)&name, &lenn)) {
        return NULL;
    }
    return inet_ntoa((struct in_addr)name.sin_addr);
}

// Функция создания хоста
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
    FILE *file_login, *file_password;
    struct sockaddr_in clientAddress;
    // char response[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>Hello, World!</h1>";

    // Инициализация Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Error %d during initialization Winsock.\n", WSAGetLastError());
        return 1;
    }

    // Создание слушающего сокета, привязка к адресу
    SOCKET listenSocket = getHost("127.0.0.2", 8080);

    while (1) {
        int len; 
        char buff[SIZE_BUF];
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
        snprintf(buff, SIZE_BUF, "Hello Client!! Welcome to ftp-server!\n\nSend \'-h\' to get help with server commands.\n");
        send(clientSocket, buff, strlen(buff), 0);
        
        do {
            // Получение сообщения
            if ((len = recv(clientSocket, (char*)&buff, SIZE_BUF, 0)) == SOCKET_ERROR) {
                return -1;
            }
            buff[len] = '\0';
            printf("Client: ");
            for (int i = 0; i < len; i++) {
                printf ("%c", buff[i]);
            }
            printf("\n");

            if (procMsg(listenSocket, clientSocket, (char*)&buff)){
                return 1;
            }

            Sleep((DWORD)10);
        } while (len != 0);

        // printf("Msg received.\n");
        // Отправка сообщения клиенту
        // send(clientSocket, response, sizeof(response) - 1, 0);

        closesocket(clientSocket);
        printf("Connection closed.\n");
    }

    closesocket(listenSocket);

    // Освобождение ресурсов Winsock
    WSACleanup();
    return 0;
}