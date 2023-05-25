#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define SIZE_BUF 512
#define LEN_LOGPASS 11
#define QTY_LOGPASS 10

#define STOP 0
#define HELP 1
#define LOGIN 2
#define REG 3
#define LIST 4
#define SEND 5 
#define GET 6
#define EXIT 7

// Обработчик команд
int procMsgr(char* buff) {
    if (!strncmp(buff, "-stop", 5))
        return STOP;
    if (!strncmp(buff, "-help", 5))
        return HELP;
    if (!strncmp(buff, "-login", 6))
        return LOGIN;
    if (!strncmp(buff, "-reg", 4))
        return REG;
    if (!strncmp(buff, "-list", 5))
        return LIST;
    if (!strncmp(buff, "-sendfile", 9))
        return SEND;
    if (!strncmp(buff, "-getfile", 8))
        return GET;
    if (!strncmp(buff, "-exit", 5))
        return EXIT; 
    return INT_MAX; // lucky
}

// Остановка сервера
void stopServ(SOCKET serv, SOCKET clnt) {
    char buff[SIZE_BUF];
    printf("Server stopped by client.\n");
    snprintf(buff, SIZE_BUF, "Server stopped... Goodbye Client! Nice to meet you again!\n");
    send(clnt, buff, strlen(buff), 0);
    closesocket(clnt);
    closesocket(serv);
}

// Отправка файла клиент -> сервер
void sendFile(SOCKET clnt, char* buff) {
    int len = 0;
    printf("Start file-transfer operation from client.\n");

    snprintf(buff, SIZE_BUF, "Send to server name of file.\n");
    send(clnt, buff, strlen(buff), 0);

    printf("Waiting for filename...\n");

    // Получение имени файла
    // ДОБАВИТЬ ОБРАБОТКУ ОШИБКИ С ДИРЕКТОРИЯМИ
    if ((len = recv(clnt, buff, SIZE_BUF, 0)) == SOCKET_ERROR) {
        printf("Error getting filename.\n");
        closesocket(clnt);
        exit(0);
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
        exit(0);
    }

    // Принятие и сохранение файла
    while (1) {
        if ((len = recv(clnt, buff, SIZE_BUF, 0)) == SOCKET_ERROR) {
            printf("File retrieval error.\n");
            fclose(file);
            closesocket(clnt);
            exit(0);
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
}

// Авторизация клиента
int authUser(SOCKET clnt, char* buff, char pass[][LEN_LOGPASS], char login[][LEN_LOGPASS], int counter) {
    int len = 0, auth = 0;
    char check[LEN_LOGPASS];
    printf("Start of the client authorization operation.\n");

    snprintf(buff, SIZE_BUF, "Start authorization operation. Send your login [%d].\n", LEN_LOGPASS - 1);
    send(clnt, buff, strlen(buff), 0);

    while (1) {
        // Получение логина
        if ((len = recv(clnt, buff, SIZE_BUF, 0)) == SOCKET_ERROR) {
            printf("Error getting login.\n");
            closesocket(clnt);
            exit(0);
        }

        // Проверка длины (переделать)
        if (len != LEN_LOGPASS - 1) {
            printf("Invalid login length. Repeating.\n");

            snprintf(buff, SIZE_BUF, "Invalid login length. Repeat sending your login.\n");
            send(clnt, buff, strlen(buff), 0);
            continue;
        }

        strncpy((char*)&check, buff, len);
        check[len] = '\0';
        printf("Recived login: \'%s\' [%d]\n", check, len);
        break;
    }

    // Проверка массива логинов на полученный логин
    for (int i = 0; i < counter; i++) {
        if (!strncmp(check, login[i], LEN_LOGPASS - 1)) {
            printf("Login \'%s\' accepted.\n", login[i]);

            snprintf(buff, SIZE_BUF, "Login accepted. Send your password.\n");
            send(clnt, buff, strlen(buff), 0);

            auth = 1;
            break;
        }
    }

    // Вывод ошибки при несуществующем логине
    if (!auth) {
        printf("User with login %s does not exist.\n", check);

        snprintf(buff, SIZE_BUF, "User with login %s does not exist. Authorization has been cancelled.\n", check);
        send(clnt, buff, strlen(buff), 0);
        return 0;
    }
    
    while (1) {
        // Получение пароля
        if ((len = recv(clnt, buff, SIZE_BUF, 0)) == SOCKET_ERROR) {
            printf("Error getting password.\n");
            closesocket(clnt);
            exit(0);
        }

        // Проверка длины (переделать)
        if (len != LEN_LOGPASS - 1) {
            printf("Invalid password length. Repeating.\n");
            snprintf(buff, SIZE_BUF, "Invalid login length. Repeat sending your password.\n");
            send(clnt, buff, strlen(buff), 0);
            continue;
        }

        strncpy((char*)&check, buff, len);
        check[len] = '\0';
        printf("Recived password: \'%s\' [%d]\n", check, len);
        break;
    }
    
    // Проверка массива паролей на полученный пароль
    auth = 0;
    for (int i = 0; i < counter; i++) {
        if (!strncmp(check, pass[i], LEN_LOGPASS - 1)) {
            auth = 1;
            printf("Password \'%s\' accepted.\n", login[i]);
            snprintf(buff, SIZE_BUF, "Password \'%s\'accepted. You are successfully logged in.\n", login[i]);
            send(clnt, buff, strlen(buff), 0);
            break;
        }
    }

    // Вывод ошибки при неверном пароле
    if (!auth) {
        printf("Password is incorrect.\n", check);
        sendToSock(clnt, buff, "Password is incorrect. Authorization has been cancelled.\n");
        return 0;
    }

    printf("Authorization successful, user logged in.\n");
    return 1;
}

// Регистрация нового пользователя
int regUser(SOCKET clnt, char* buff, char pass[][LEN_LOGPASS], char login[][LEN_LOGPASS], int counter) {
    if (counter >= QTY_LOGPASS) {
        printf("Registration error! Users cache [%d] overflow. Creating a new user is not possible.\n", QTY_LOGPASS);

        snprintf(buff, SIZE_BUF, "Registration error! Users cache [%d] overflow. Creating a new user is not possible.\n", LEN_LOGPASS - 1);
        send(clnt, buff, strlen(buff), 0);
    }
    int len = 0;
    printf("Start of the client registration operation.\n");
    snprintf(buff, SIZE_BUF, "Start registration operation. Send your login [%d].\n", LEN_LOGPASS - 1);
    send(clnt, buff, strlen(buff), 0);

    while (1) {
        // Получение нового логина
        if ((len = recv(clnt, buff, SIZE_BUF, 0)) == SOCKET_ERROR) {
            printf("Error getting login.\n");
            closesocket(clnt);
            exit(0);
        }

        // Проверка длины (переделать)
        if (len != LEN_LOGPASS - 1) {
            printf("Invalid login length. Repeating.\n");
            sendToSock(clnt, buff, "Invalid login length. Repeat sending your login.\n");
            continue;
        }
        
        // Проверка на существующий логин
        for (int i = 0; i < counter; i++) {
            if (!strncmp(buff, login[i], LEN_LOGPASS - 1)) {
                printf("Error! Login \'%s\' already exists. Reg closed.\n", login[i]);
                snprintf(buff, SIZE_BUF, "Error! Login \'%s\' already exists. Registration has been cancelled.\n", login[i]);
                send(clnt, buff, strlen(buff), 0);
                return counter;
            }
        }

        strncpy((char*)&login[counter], buff, len);
        login[counter][len] = '\0';
        printf("Recived login: \'%s\' [%d]\n", login[counter], len);
        break;
    }

    printf("Login \'%s\' accepted.\n", login[counter]);
    snprintf(buff, SIZE_BUF, "Login \'%s\' accepted.\n", login[counter]);
    send(clnt, buff, strlen(buff), 0);

    while (1) {
        // Получение пароля для регистрации
        if ((len = recv(clnt, buff, SIZE_BUF, 0)) == SOCKET_ERROR) {
            printf("Error getting password.\n");
            closesocket(clnt);
            exit(0);
        }

        // Проверка длины (переделать)
        if (len != LEN_LOGPASS - 1) {
            printf("Invalid password length. Repeating.\n");
            sendToSock(clnt, buff, "Invalid login length. Repeat sending your password.\n");
            continue;
        }

        strncpy((char*)&pass[counter], buff, len);
        pass[counter][len] = '\0';
        printf("Recived password: %s [%d]\n", pass[counter], len);
        break;
    }
    printf("Password \'%s\' accepted. Start saving.\n", login[counter]);

    counter++; // Добавление к счётчику

    // Сохранение файла с новыми данными логина
    FILE *file = fopen("./auth_files/file_login.txt", "w");
    fprintf(file, "%d", counter);
    fputs("\n", file);
    for (int i = 0; i < counter; i++) {
        fwrite(login[i], sizeof(login[i]) - 1, 1, file);
        fputs("|", file);
    }
    fclose(file);

    // Сохранение файла с новыми данными пароля
    file = fopen("./auth_files/file_password.txt", "w");
    fprintf(file, "%d", counter);
    fputs("\n", file);
    for (int i = 0; i < counter; i++) {
        fwrite(pass[i], sizeof(pass[i]) - 1, 1, file);
        fputs("|", file);
    }
    fclose(file);

    sendToSock(clnt, buff, "Password accepted. Successfully registration. Please login.\n");
    return counter;
}

// Выгрузка файла в массив
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
            fread(arr[i], sizeof(arr[i]) - 1, 1, file);
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

void sendToSock(SOCKET clnt, char* buff, char* text) {
    snprintf(buff, SIZE_BUF, text);
    send(clnt, buff, strlen(buff), 0);
}

// Функция создания хоста
SOCKET getHost(char* ipAddr, int port) {
    SOCKET s;
    struct sockaddr_in serverAddress;
    // Создание слушающего сокета
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Failed to create listening socket. Error %d\n", WSAGetLastError());
        exit(0);
    }

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
        // snprintf(buff, SIZE_BUF, "Welcome to ftp-server!\n\nSend \'-help\' to get help with server commands.\n");
        // send(clientSocket, buff, strlen(buff), 0);
        sendToSock(clientSocket, buff, "Welcome to ftp-server!\n\nSend \'-help\' to get help with server commands.\n");
        
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
                case STOP: // -stop Секретная команда
                    stopServ(listenSocket, clientSocket);
                    return 1;
                case HELP: // -help Вывод помощи по командам
                    snprintf(buff, SIZE_BUF, "\n-stop\t\tStopped server.\n-help\t\tShows help-manual.\n-list\t\tList of files.\n-sendfile\tTransfer from client to server.\n-getfile\tTransfer from server to client.\n-login\t\tClient authorization.\n-reg\t\tNew client registration.\n");
                    send(clientSocket, buff, strlen(buff), 0);
                    break;
                case LOGIN: // -login Авторизация клиента
                    auth_flag = authUser(clientSocket, (char*)&buff, password, login, log_counter);
                    break;
                case REG: // -reg Регистрация нового клиента
                    log_counter = regUser(clientSocket, (char*)&buff, password, login, log_counter);
                    break;
                case LIST: // -list Вывод каталога файлов
                    printf("None.\n");
                    break;
                case SEND: // -sendfile Отправка файла клиент -> сервер
                    sendFile(clientSocket, (char*)&buff);
                    break;
                case GET: // -getfile Отправка файла сервер -> клиент
                    printf("None.\n");
                    break;
                case EXIT: // -exit Прекращение сессии
                    printf("None.\n");
                    break;
                default: // -
                    snprintf(buff, SIZE_BUF, "No such command found. Type -help for help.\n");
                    send(clientSocket, buff, strlen(buff), 0);
                    break;
                }
            } else {
                snprintf(buff, SIZE_BUF, "Msg received to client.\n");
                if (send(clientSocket, buff, strlen(buff), 0) == SOCKET_ERROR) {
                    closesocket(clientSocket);
                    break;
                }
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