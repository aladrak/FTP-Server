#include "server_lib.h"

#define IP_ADDR "127.0.0.2"
#define PORT 8080

int main() {
    int auth_user = NO_LOGGED;
    char login[QTY_LOGPASS][LEN_LOGPASS], 
    password[QTY_LOGPASS][LEN_LOGPASS];

// Создание каталогов для файлов и выгрузка logpass
    int log_counter = createDir(login, password);

    // char response[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>Hello, World!</h1>";

    // Инициализация Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Error %d during initialization Winsock.\n", WSAGetLastError());
        return 1;
    }

// Создание слушающего сокета, привязка к адресу
    SOCKET listenSocket = getHost(IP_ADDR, PORT);

    while (1) {
        int len; 
        char buff[SIZE_BUF];
        printf("The server is up at %s and waiting for connections...\n", getSockIp(listenSocket));

        // Принятие входящего соединения
        struct sockaddr_in clientAddress;
        int clientAddressSize = sizeof(clientAddress);
        SOCKET clientSocket = accept(listenSocket, (struct sockaddr*)&clientAddress, &clientAddressSize);
        if (clientSocket == INVALID_SOCKET) {
            printf("An error occurred while accepting an incoming connection.\n");
            closesocket(listenSocket);
            return 1;
        }
        printf("New connection with %s received.\n", getSockIp(clientSocket));

        // Отправка сообщения клиенту
        snprintf(buff, SIZE_BUF, "Welcome to ftp-server!\n\nSend \'-help\' to get help with server commands.\n");
        send(clientSocket, buff, strlen(buff), 0);
        
        do {
            // Получение сообщения
            if ((len = recv(clientSocket, (char*)&buff, SIZE_BUF, 0)) == SOCKET_ERROR) {
                break;
            }
            buff[len] = '\0';
            printf("Client: ");
            for (int i = 0; i < len; i++) {
                printf("%c", buff[i]);
            }
            printf("\n");
            
            // Обработка сообщений и выполнение команд
            if (buff[0] == '-') {
                switch (procMsgr(buff)) {
                case CODE_STOP: // -stop Секретная команда
                    stopServ(listenSocket, clientSocket);
                    return 1;
                case CODE_HELP: // -help Вывод помощи по командам
                    snprintf(buff, SIZE_BUF, "\n-stop\t\tStopped server.\n-help\t\tShows help-manual.\n-login\t\tClient authorization.\n-reg\t\tNew client registration.\n-list\t\tList of files.\n-sendfile <filename>\tTransfer file from client to server.\n-getfile <filename>\tTransfer file from server to client.\n-remove <filename>\tRemoving a file from a directory.\n-exit\t\tEnd of session.\n");
                    send(clientSocket, buff, strlen(buff), 0);
                    break;
                case CODE_LOGIN: // -login Авторизация клиента
                    auth_user = authUser(clientSocket, (char*)&buff, password, login, log_counter, auth_user);
                    break;
                case CODE_REG: // -reg Регистрация нового клиента
                    log_counter = regUser(clientSocket, (char*)&buff, password, login, log_counter, auth_user);
                    break;
                case CODE_LIST: // -list Вывод каталога файлов
                    showList(clientSocket, (char*)&buff, login, auth_user);
                    break;
                case CODE_SEND: // -sendfile Отправка файла клиент -> сервер
                    sendFileCom(clientSocket, (char*)&buff, login, auth_user);
                    break;
                case CODE_GET: // -getfile Отправка файла сервер -> клиент
                    getFileCom(clientSocket, (char*)&buff, login, auth_user);
                    break;
                case CODE_REMOVE: // -remove Удаление файла
                    removeFileCom(clientSocket, (char*)&buff, login, auth_user);
                    break;
                case CODE_EXIT: // -exit Прекращение сессии
                    auth_user = NO_LOGGED;
                    snprintf(buff, SIZE_BUF, "EXIT");
                    send(clientSocket, buff, strlen(buff), 0);
                    closesocket(clientSocket);
                    break;
                default: // -
                    snprintf(buff, SIZE_BUF, "No such command found. Type -help for help.\n");
                    send(clientSocket, buff, strlen(buff), 0);
                    break;
                }
            } else {
                snprintf(buff, SIZE_BUF, "The message is received.\n");
                if (send(clientSocket, buff, strlen(buff), 0) == SOCKET_ERROR) {
                    closesocket(clientSocket);
                    break;
                }
            }

            Sleep((DWORD)10);
        } while (len != 0 || clientSocket != SOCKET_ERROR);

        // printf("Msg received.\n");
        // Отправка сообщения клиенту
        // send(clientSocket, response, sizeof(response) - 1, 0);
        auth_user = NO_LOGGED;
        closesocket(clientSocket);
        printf("Connection closed.\n");
    }

    closesocket(listenSocket);

    // Освобождение ресурсов Winsock
    WSACleanup();
    return 0;
}