#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <dirent.h>

#define SIZE_BUF 512
#define LEN_LOGPASS 11
#define QTY_LOGPASS 10
#define LEN_FILENAME 25

#define FOLDER_AUTH "./auth_files"
#define FOLDER_FILES "./stored_files"
#define FOLDER_SHARED "/shared"

#define NO_LOGGED -1

#define CODE_STOP 0
#define CODE_HELP 1
#define CODE_LOGIN 2
#define CODE_REG 3
#define CODE_LIST 4
#define CODE_SEND 5 
#define CODE_GET 6
#define CODE_REMOVE 7
#define CODE_EXIT 8

int getNextChar(char* str1, char *str2) {
	char *p;
	 if ((p = strstr(str1, str2)) != NULL)
        return(p - str1 + strlen(str2));
    return 0;
}

// Обработчик команд
int procMsgr(char* buff) {
    if (!strncmp(buff, "-stop", 5))
        return CODE_STOP;
    if (!strncmp(buff, "-help", 5))
        return CODE_HELP;
    if (!strncmp(buff, "-login", 6))
        return CODE_LOGIN;
    if (!strncmp(buff, "-reg", 4))
        return CODE_REG;
    if (!strncmp(buff, "-list", 5))
        return CODE_LIST;
    if (!strncmp(buff, "-sendfile", 9))
        return CODE_SEND;
    if (!strncmp(buff, "-getfile", 8))
        return CODE_GET;
    if (!strncmp(buff, "-remove", 7))
        return CODE_REMOVE;
    if (!strncmp(buff, "-exit", 5))
        return CODE_EXIT; 
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

// Отправка списка файлов в директории пользователя
void showList(SOCKET clnt, char* buff, char login[][LEN_LOGPASS], int auth_user) {
    // Проверка на вход
    if (auth_user == -1) {
        snprintf(buff, SIZE_BUF, "-end");
        send(clnt, buff, strlen(buff), 0);

        printf("The client is not logged in.\n");
        snprintf(buff, SIZE_BUF, "Log in before using this command.\n");
        send(clnt, buff, strlen(buff), 0);
        return;
    }

    snprintf(buff, SIZE_BUF, "%s/%s", FOLDER_FILES, login[auth_user]);
    buff[strlen(buff)] = '\0';
    char tempdir[strlen(buff)];
    strcpy(tempdir, buff);

    int j = 0;
    DIR* dir = opendir(tempdir);
    if (!dir) {
        perror("diropen");
    }
    snprintf(buff, SIZE_BUF, "List of files in directory \'%s\':\n", tempdir);
    send(clnt, buff, strlen(buff), 0);

    struct dirent* filename;
    while ((filename = readdir(dir)) != NULL) {
        if (strlen(filename->d_name) > 2) {
            printf("%s\n", filename->d_name);
            snprintf(buff, SIZE_BUF, "%s\n", filename->d_name);
            buff[strlen(buff)] = '\0';
            send(clnt, buff, strlen(buff), 0);
            j++;
        }
    }
    closedir(dir);
    
    if (j == 0) {
        snprintf(buff, SIZE_BUF, "No files in the directory.\n");
        send(clnt, buff, strlen(buff), 0);
    }
    //end sending
    Sleep((DWORD)20);
    snprintf(buff, SIZE_BUF, "-end");
    send(clnt, buff, strlen(buff), 0);

    snprintf(buff, SIZE_BUF, "List ended.\n");
    send(clnt, buff, strlen(buff), 0);
    free(filename);
}

// Получение файла клиент -> сервер
void sendFile(SOCKET clnt, char* buff, int auth_user) {
    // Проверка на вход
    if (auth_user == NO_LOGGED) {
        // snprintf(buff, SIZE_BUF, "-end");
        // send(clnt, buff, strlen(buff), 0);

        printf("The client is not logged in.\n");
        snprintf(buff, SIZE_BUF, "Log in before using this command.\n");
        send(clnt, buff, strlen(buff), 0);
        return;
    }

    int len = 0;
    printf("Start file-transfer operation from client.\n");
    snprintf(buff, SIZE_BUF, "Send your filename [%d].\n", LEN_FILENAME);
    send(clnt, buff, strlen(buff), 0);

    printf("Waiting for filename...\n");

    while (1) {
    // Получение имени файла
        if ((len = recv(clnt, buff, SIZE_BUF, 0)) == SOCKET_ERROR) {
            printf("Error getting filename.\n");
            closesocket(clnt);
            exit(0);
        }
        printf("Recived filename: ");
        printf("%s\n", buff);

        // Проверка на неправильные символы
        if ((strstr(buff, "\\") != NULL) || (strstr(buff, "/") != NULL)) {
            printf("Wrong characters in the filename. Repeating.\n");
            snprintf(buff, SIZE_BUF, "Wrong characters in the filename. Repeat sending your filename.\n");
            send(clnt, buff, strlen(buff), 0);
            continue;
        }
        
        // Проверка длины
        if (len > LEN_FILENAME) {
            printf("Invalid filename length. Repeating.\n");
            snprintf(buff, SIZE_BUF, "Invalid filename length. Repeat sending your filename.\n");
            send(clnt, buff, strlen(buff), 0);
            continue;
        }
        break;
    }

    char filename[128];
    strncpy((char*)&filename, buff, len);
    filename[len] = '\0';

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

void getFileCom(SOCKET clnt, char* buff, char login[][LEN_LOGPASS], int auth_user) {
    char filename[strlen(buff) - 8];
    // snprintf(filename, 128, "");
    int j = 0;
    for (int i = getNextChar(buff, "-getfile"); i < strlen(buff); i++) {
        if (buff[i] == ' ') { continue; }
        filename[j++] = buff[i];
        printf(" %d ", j);  
    }
    filename[j] = '\0';
    printf("Recived filename \'%s\' fnlen%d buflen%d .\n", filename, strlen(filename), strlen(buff));
    exit(0);
}

// Отправка файла сервер -> клиент
void getFile(SOCKET clnt, char* buff, char login[][LEN_LOGPASS], int auth_user){
    // Проверка на вход
    if (auth_user == NO_LOGGED) {
        snprintf(buff, SIZE_BUF, "-end");
        send(clnt, buff, strlen(buff), 0);

        Sleep((DWORD)25);

        printf("The client is not logged in.\n");
        snprintf(buff, SIZE_BUF, "Log in before using this command.\n");
        send(clnt, buff, strlen(buff), 0);
        return;
    }

    int len = 0;
    char recvfilename[LEN_FILENAME + 1];
    char tempdir[LEN_LOGPASS + strlen(FOLDER_FILES)];
    printf("Start file-transfer operation from server.\n");
    snprintf(buff, SIZE_BUF, "Send the file name you require [%d].\n", LEN_FILENAME);
    send(clnt, buff, strlen(buff), 0);

    while (1) {
        // Получение имени файла
        if ((len = recv(clnt, buff, SIZE_BUF, 0)) == SOCKET_ERROR) {
            printf("Error getting filename.\n");
            closesocket(clnt);
            exit(0);
        }
        buff[len] = '\0';
        printf("Recived filename: \'%s\', %d\n", buff, len);
        strcpy(recvfilename, buff);

        // Проверка длины
        if (len > LEN_FILENAME && len < 2) {
            snprintf(buff, SIZE_BUF, "-end");
            send(clnt, buff, strlen(buff), 0);

            Sleep((DWORD)25);

            printf("Invalid filename length.\n");
            snprintf(buff, SIZE_BUF, "Invalid filename length. Canceling.\n");
            send(clnt, buff, strlen(buff), 0);
            return;
        }

        snprintf(buff, SIZE_BUF, "%s/%s/", FOLDER_FILES, login[auth_user]);
        buff[strlen(buff)] = '\0';
        strcpy(tempdir, buff);

        DIR* dir = opendir(tempdir);
        if (!dir) {
            perror("diropen");
        }
        // Поиск нужного файла в директории пользователя
        struct dirent* filename;
        while ((filename = readdir(dir)) != NULL) {
            if (strlen(filename->d_name) > 2 && !strncmp(filename->d_name, recvfilename, strlen(recvfilename))) {
                printf("A valid filename has been received. Start of file transfer.\n");
                snprintf(buff, SIZE_BUF, "A valid filename has been received. Start of file transfer.\n");
                send(clnt, buff, strlen(buff), 0);
                break;
            }
        }
        closedir(dir);
        if (filename->d_name == NULL){
            printf("No such file exists. Canceling the operation.\n");
            snprintf(buff, SIZE_BUF, "-end No such file exists. Canceling the operation.\n");
            send(clnt, buff, strlen(buff), 0);
            return;
        }
        break;
    }

    snprintf(buff, SIZE_BUF, "%s%s", tempdir, recvfilename);

    FILE *file = fopen(buff, "rb");
    if (file == NULL) {
        printf("Failed to open file for reading.\n");
        closesocket(clnt);
        return;
    }

    while (1) {
        int bytesRead = fread(buff, 1, SIZE_BUF, file);
        if (bytesRead > 0) {
            send(clnt, buff, bytesRead, 0);
        } else {
            break;
        }
    }
    snprintf(buff, SIZE_BUF, "-end");
    send(clnt, buff, strlen(buff), 0);

    snprintf(buff, SIZE_BUF, "Successful sending of the file \'%s%s\' to the client.\n", tempdir, recvfilename);
    send(clnt, buff, strlen(buff), 0);

    fclose(file);
}

// Авторизация клиента
int authUser(SOCKET clnt, char* buff, char pass[][LEN_LOGPASS], char login[][LEN_LOGPASS], int counter, int auth_user) {
    // Проверка на уже выполненный вход
    if (auth_user != NO_LOGGED) {
        printf("User is already logged in. Authorization has been cancelled.\n");
        snprintf(buff, SIZE_BUF, "You are already logged in. Authorization has been cancelled.\n", LEN_LOGPASS - 1);
        send(clnt, buff, strlen(buff), 0);
        return auth_user;
    }

    int len = 0, auth = NO_LOGGED;
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

            auth = i;
            break;
        }
    }

    // Вывод ошибки при несуществующем логине
    if (auth == NO_LOGGED) {
        printf("User with login %s does not exist.\n", check);
        snprintf(buff, SIZE_BUF, "User with login %s does not exist. Authorization has been cancelled.\n", check);
        send(clnt, buff, strlen(buff), 0);
        return NO_LOGGED;
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
    
    // Проверка пароля на соотношение с логином
    if (!strncmp(check, pass[auth], LEN_LOGPASS - 1)) {
        printf("Password for \'%s\' accepted.\n", login[auth]);
        snprintf(buff, SIZE_BUF, "Password for \'%s\'accepted. You are successfully logged in.\n", login[auth]);
        send(clnt, buff, strlen(buff), 0);
    }
    // Вывод ошибки при неверном пароле
    else {
        printf("Password for \'%s\'is incorrect.\n", login[auth]);
        snprintf(buff, SIZE_BUF, "Password for \'%s\' is incorrect. Authorization has been cancelled.\n", login[auth]);
        send(clnt, buff, strlen(buff), 0);
        return NO_LOGGED;
    }

    printf("Authorization successful, user logged in.\n");
    return auth;
}

// Регистрация нового пользователя
int regUser(SOCKET clnt, char* buff, char pass[][LEN_LOGPASS], char login[][LEN_LOGPASS], int counter, int auth_user) {
    // Проверка на переполнение кэша логинов/паролей
    if (counter >= QTY_LOGPASS) {
        printf("Registration error! Users cache [%d] overflow. Creating a new user is not possible.\n", QTY_LOGPASS);
        snprintf(buff, SIZE_BUF, "Registration error! Users cache [%d] overflow. Creating a new user is not possible.\n", QTY_LOGPASS);
        send(clnt, buff, strlen(buff), 0);
        return counter;
    }

    // Проверка на уже выполненный вход
    if (auth_user != NO_LOGGED) {
        printf("User is already logged in. Registration has been cancelled.\n");
        snprintf(buff, SIZE_BUF, "You are already logged in. Registration has been cancelled.\n", LEN_LOGPASS - 1);
        send(clnt, buff, strlen(buff), 0);
        return counter;
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
            snprintf(buff, SIZE_BUF, "Invalid login length. Repeat sending your login.\n");
            send(clnt, buff, strlen(buff), 0);
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
            snprintf(buff, SIZE_BUF, "Invalid login length. Repeat sending your password.\n");
            send(clnt, buff, strlen(buff), 0);
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
    snprintf(buff, SIZE_BUF, "%s/file_login.txt", FOLDER_AUTH);
    buff[strlen(buff)] = '\0';
    FILE *file = fopen(buff, "w");
    fprintf(file, "%d", counter);
    fputs("\n", file);
    for (int i = 0; i < counter; i++) {
        fwrite(login[i], sizeof(login[i]) - 1, 1, file);
        fputs("|", file);
    }
    fclose(file);

    // Сохранение файла с новыми данными пароля
    snprintf(buff, SIZE_BUF, "%s/file_password.txt", FOLDER_AUTH);
    buff[strlen(buff)] = '\0';
    file = fopen(buff, "w");
    fprintf(file, "%d", counter);
    fputs("\n", file);
    for (int i = 0; i < counter; i++) {
        fwrite(pass[i], sizeof(pass[i]) - 1, 1, file);
        fputs("|", file);
    }
    fclose(file);

    // Создание каталога для нового пользователя
    snprintf(buff, SIZE_BUF, "%s/", FOLDER_FILES);
    strcat(buff, login[counter - 1]);
    buff[strlen(buff)] = '\0';
    if (!mkdir(buff)) {
        printf("New dir \'%s\' created.\n", buff);
    } else {
        printf("Dir \'%s\' already exists.\n", buff);
    }

    snprintf(buff, SIZE_BUF, "Password accepted. Successfully registration. Please login.\n");
    send(clnt, buff, strlen(buff), 0);
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
            printf("%s [%d]\n", arr[i], strlen(arr[i]));
        }
    } else {
        printf("File open error.\n");
        exit(0);
    }
    fclose(file);
    printf("File \'%s\' initialized.\n", path);
    return counter;
}

// Создание системных и файловых каталогов, выгрузка passlog файлов
int createDir(char login[][LEN_LOGPASS], char password[][LEN_LOGPASS]) {
    int log_counter = 0;
    char tempdir[128];

    // Создание каталога паролей
    if (!mkdir(FOLDER_AUTH)) {
        printf("Dir \'%s\' created.\n", FOLDER_AUTH);
    } else { 
        printf("Dir \'%s\' already exists. Start scaning passlog files.\n", FOLDER_AUTH); 

    // Выборка сохранённых логинов и занесение в массив
        snprintf(tempdir, 128, "%s/file_login.txt", FOLDER_AUTH);
        tempdir[strlen(tempdir)] = '\0';
        log_counter = uploadFile(login, (char*)&tempdir);

    // Выборка сохранённых паролей и занесение в массив
        snprintf(tempdir, 128, "%s/file_password.txt", FOLDER_AUTH);
        tempdir[strlen(tempdir)] = '\0';
        if (log_counter != uploadFile(password, (char*)&tempdir)) {
            printf("Error. Too many data in ratio in logpass files!\n");
            exit(0);
        }
    }
    
    // Создание каталога файлов
    if (!mkdir(FOLDER_FILES)) {
        printf("Dir \'%s\' created.\n", FOLDER_FILES);
        for (int i = 0; i < log_counter; i++) {
            snprintf(tempdir, 128, "%s/", FOLDER_FILES);
            strcat(tempdir, login[i]);
            tempdir[strlen(tempdir)] = '\0';
            mkdir(tempdir);
            printf("Dir \'%s\' created.\n", tempdir);
        }
    } else { 
        printf("Dir \'%s\' already exists. Start scaning files.\n", FOLDER_FILES);
        for(int i = 0; i < log_counter; i++){
            snprintf(tempdir, 128, "%s/", FOLDER_FILES);
            strcat(tempdir, login[i]);
            tempdir[strlen(tempdir)] = '\0';
            if (!mkdir(tempdir)) {
                printf("Dir \'%s\' created.\n", tempdir);
            } else {
                printf("Dir \'%s\' already exists.\n", tempdir);
            }
        }
    }
    // Создание общей папки
    snprintf(tempdir, 128, "%s%s", FOLDER_FILES, FOLDER_SHARED);
    tempdir[strlen(tempdir)] = '\0';
    if (!mkdir(tempdir)) {
        printf("Dir \'%s\' created.\n", tempdir);
    } else {
        printf("Dir \'%s\' already exists.\n", tempdir);
    }

    return log_counter;
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
    int auth_user = NO_LOGGED, log_counter = 0;
    char login[QTY_LOGPASS][LEN_LOGPASS], password[QTY_LOGPASS][LEN_LOGPASS];
    struct sockaddr_in clientAddress;

// Создание каталогов для файлов и выгрузка logpass
    log_counter = createDir(login, password);

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
                case CODE_STOP: // -stop Секретная команда
                    stopServ(listenSocket, clientSocket);
                    return 1;
                case CODE_HELP: // -help Вывод помощи по командам
                    snprintf(buff, SIZE_BUF, "\n-stop\t\tStopped server.\n-help\t\tShows help-manual.\n-login\t\tClient authorization.\n-reg\t\tNew client registration.\n-list\t\tList of files.\n-sendfile\tTransfer file from client to server.\n-getfile\tTransfer file from server to client.\n-exit\t\tEnd of session.\n");
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
                    sendFile(clientSocket, (char*)&buff, auth_user);
                    break;
                case CODE_GET: // -getfile Отправка файла сервер -> клиент
                    getFileCom(clientSocket, (char*)&buff, login, auth_user);
                    //getFile(clientSocket, (char*)&buff, login, auth_user);
                    break;
                case CODE_REMOVE:
                    break;
                case CODE_EXIT: // -exit Прекращение сессии
                    closesocket(clientSocket);
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
        } while (len != 0 || clientSocket != SOCKET_ERROR);

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