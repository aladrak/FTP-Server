#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define SIZE_BUF 512
#define LEN_LOGPASS 11
#define QTY_LOGPASS 10

int procMsg(SOCKET serv, SOCKET clnt, char* buff, char pass[][LEN_LOGPASS], char login[][LEN_LOGPASS], int counter){
    if (buff[0] == '-') {
// Остановка сервера
        if (!strncmp(buff, "-stop", 5)) {
            printf("Server stopped by client.\n");
            snprintf(buff, SIZE_BUF, "Server stopped... Goodbye Client! Nice to meet you again!\n");
            send(clnt, buff, strlen(buff), 0);
            closesocket(clnt);
            closesocket(serv);
            return 1;
// Вывод помощи по командам
        } else if (!strncmp(buff, "-help", 5)) {
            snprintf(buff, SIZE_BUF, "\n-stop\t\tStopped server.\n-help\t\tHelp-manual.\n-list\t\tList of files.\n-sendfile\tTransfer from client to server.\n-getfile\tTransfer from server to client.\n-login\tClient authorization.\n-reg\tNew client registration.\n");
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

// Вывод каталога файлов
        } else if (!strncmp(buff, "-list", 5)) {
            
// Авторизация клиента
        } else if (!strncmp(buff, "-login", 6)) {
            int len = 0, auth_login = 0, auth_pass = 0;
            printf("Start of the client authorization operation.\n");
            snprintf(buff, SIZE_BUF, "Start authorization operation. Send your login [%d].\n", LEN_LOGPASS - 1);
            send(clnt, buff, strlen(buff), 0);
            char check[LEN_LOGPASS];
            while (1) {
                // Проверка логина
                if ((len = recv(clnt, buff, SIZE_BUF, 0)) == SOCKET_ERROR) {
                    printf("Error getting login.\n");
                    closesocket(clnt);
                    return 1;
                }
    
                strncpy((char*)&check, buff, len);
                check[len] = '\0';
                printf("Recived login: %s [%d]\n", check, len);
                // Проверка длины (переделать)
                if (len != LEN_LOGPASS - 1) {
                    printf("Invalid login length. Repeating.\n");
                    snprintf(buff, SIZE_BUF, "Invalid login length. Repeat sending your login.\n");
                    send(clnt, buff, strlen(buff), 0);
                    continue;
                }
                break;
            }
        
            // Проверка массива логинов на полученный логин
            for (int i = 0; i < counter; i++) {
                if (!strncmp(check, login[i], LEN_LOGPASS - 1)) {
                    auth_login = 1;
                    printf("Login %s accepted.\n", login[i]);
                    snprintf(buff, SIZE_BUF, "Login accepted. Send your password.\n");
                    send(clnt, buff, strlen(buff), 0);
                    break;
                }
            }
            // Вывод ошибки при несуществующем логине
            if (!auth_login) {
                printf("User with login %s does not exist.\n", check);
                snprintf(buff, SIZE_BUF, "User with login %s does not exist.\n", check);
                send(clnt, buff, strlen(buff), 0);
                return 0;
            }

            while (1) {
                // Проверка пароля
                if ((len = recv(clnt, buff, SIZE_BUF, 0)) == SOCKET_ERROR) {
                    printf("Error getting password.\n");
                    closesocket(clnt);
                    return 1;
                }
    
                strncpy((char*)&check, buff, len);
                check[len] = '\0';
                printf("Recived password: %s [%d]\n", check, len);
                // Проверка длины (переделать)
                if (len != LEN_LOGPASS - 1) {
                    printf("Invalid password length. Repeating.\n");
                    snprintf(buff, SIZE_BUF, "Invalid login length. Repeat sending your password.\n");
                    send(clnt, buff, strlen(buff), 0);
                    continue;
                }
                break;
            }
        
            // Проверка массива паролей на полученный пароль
            for (int i = 0; i < counter; i++) {
                if (!strncmp(check, pass[i], LEN_LOGPASS - 1)) {
                    auth_pass = 1;
                    printf("Password %s accepted.\n", login[i]);
                    snprintf(buff, SIZE_BUF, "Password accepted. You are successfully logged in.\n");
                    send(clnt, buff, strlen(buff), 0);
                    break;
                }
            }

            // Вывод ошибки при неверном пароле
            if (!auth_pass) {
                printf("Password is incorrect.\n", check);
                snprintf(buff, SIZE_BUF, "Password is incorrect.\n");
                send(clnt, buff, strlen(buff), 0);
                return 0;
            }
// Отправка ошибки о несуществующей команде
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

int uploadFile(char arr[][LEN_LOGPASS], char *path) {
    FILE *file;
    int counter = 0;
    char trash;
    file = fopen(path, "r");
    if (!feof(file)) {
        fscanf(file, "%d", &counter);
        fscanf(file, "%c", &trash);
        if (counter >= QTY_LOGPASS) {
            printf("Too many data in file. Need more buffer!\n");
            exit(0);
        }
        for (int i = 0; i < counter; i++) {
            fread(arr[i], sizeof(arr[i])-1, 1, file);
            arr[i][LEN_LOGPASS-1] = '\0';
            fscanf(file, "%c", &trash);
            printf("%s len%d szof%d\n", arr[i], strlen(arr[i]), sizeof(arr[i]));
        }
    } else {
        printf("File open error.\n");
        exit(0);
    }
    fclose(file);
    printf("File \'%s\' initialized.\n", path);
    return counter;
}

// Получение адреса
char* getSockIp(SOCKET s){
    struct sockaddr_in name;
    int lenn = sizeof(name);
    ZeroMemory(&name, sizeof (name));
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
    int auth_flag = 0, log_counter = 0;
    char login[QTY_LOGPASS][LEN_LOGPASS], password[QTY_LOGPASS][LEN_LOGPASS];
    struct sockaddr_in clientAddress;
    
// Выборка сохранённых логинов и занесение в массив
    log_counter = uploadFile(login, "./auth_files/file_login.txt");

// Выборка сохранённых паролей и занесение в массив
    if (log_counter != uploadFile(password, "./auth_files/file_password.txt")) {
        printf("Error. Too many data in ratio in logpass files!\n");
        return 0;
    }
    
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
        snprintf(buff, SIZE_BUF, "Welcome to ftp-server!\n\nSend \'-help\' to get help with server commands.\n");
        send(clientSocket, buff, strlen(buff), 0);
        
        do {
            // Получение сообщения
            if ((len = recv(clientSocket, (char*)&buff, SIZE_BUF, 0)) == SOCKET_ERROR) {
                return -1;
            }
            buff[len] = '\0';
            printf("Client: ");
            for (int i = 0; i < len; i++) {
                printf("%c", buff[i]);
            }
            printf("\n");
            
            // Обработка сообщений, выполнение команд
            if (procMsg(listenSocket, clientSocket, (char*)&buff, password, login, log_counter)){
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