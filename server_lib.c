
#include <dirent.h>
#include "server_lib.h"

#define WRONG_CHARS "\\/*[]{}()\'\"^%#№$@!?&:;=+`~"

#define FOLDER_AUTH "./auth_files"
#define FOLDER_FILES "./stored_files"
#define FOLDER_SHARED "/shared"

// Подсчёт кол-ва символов в строке
int getQtyChar(char* str1, char str2) {
    int j = 0;
    for (int i = 0; i < strlen(str1); i++){
        if (str1[i] == str2)
            j++;
    }
    return j;
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

// Проверка на вход/символы/корректность команды
int checkFilename(SOCKET s, char* buff, char* command, int auth_user) {
    // Проверка на вход
    if (auth_user == NO_LOGGED) {
        printf("The client is not logged in.\n");
        snprintf(buff, SIZE_BUF, "Log in before using this command.\n");
        send(s, buff, strlen(buff), 0);
        return 1;
    }

    // Проверка на неправильные символы
    if ((strpbrk(buff, WRONG_CHARS) != NULL)) {
        printf("Wrong characters in the filename.\n");
        snprintf(buff, SIZE_BUF, "Wrong characters in the filename.\n");
        send(s, buff, strlen(buff), 0);
        return 1;
    }

    // Проверка на правильность ввода команды
    if (buff[strlen(command)] != ' ') {
        printf("Received an invalid filename.\n");
        snprintf(buff, SIZE_BUF, "Received an invalid filename.\n");
        send(s, buff, strlen(buff), 0);
        return 1;
    }

    return 0; // Успех
}

// Остановка сервера
void stopServ(SOCKET serv, SOCKET clnt) {
    char buff[SIZE_BUF];
    printf("Server stopped by client.\n");
    snprintf(buff, SIZE_BUF, "STOP");
    send(clnt, buff, strlen(buff), 0);
    closesocket(clnt);
    closesocket(serv);
}

// Отправка списка файлов в директории пользователя
void showList(SOCKET clnt, char* buff, char login[][LEN_LOGPASS], int auth_user) {
    // Проверка на вход
    if (auth_user == NO_LOGGED) {
        printf("The client is not logged in.\n");
        snprintf(buff, SIZE_BUF, "Log in before using this command.\n");
        send(clnt, buff, strlen(buff), 0);
        return;
    }

    snprintf(buff, SIZE_BUF, "LIST");
    send(clnt, buff, strlen(buff), 0);
    
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

    Sleep((DWORD)30);

    snprintf(buff, SIZE_BUF, "-END");
    send(clnt, buff, strlen(buff), 0);

    snprintf(buff, SIZE_BUF, "The list of files has been sent successfully.\n");
    send(clnt, buff, strlen(buff), 0);
    free(filename);
}

// Удаление файла в директории пользователя
void removeFileCom(SOCKET clnt, char* buff, char login[][LEN_LOGPASS], int auth_user) {
    // Проверка на вход/символы/корректность команды
    if (checkFilename(clnt, buff, "-remove", auth_user)) {
        return;
    }

    // Выборка названия файла из вызова команды
    char filename[strlen(buff) - sizeof("-remove") - getQtyChar(buff, ' ') + 2];
    int j = 0;
    for (int i = sizeof("-remove"); i < strlen(buff); i++) {
        if (buff[i] == ' ') { continue; }
        filename[j++] = buff[i];
    }
    filename[j] = '\0';
    printf("[DB] Filename: \'%s\' fnlen%d fnsz%d buflen%d.\n", filename, strlen(filename), sizeof(filename), strlen(buff));

    // Проверка длины
    if (sizeof(filename) > LEN_FILENAME || sizeof(filename) < 2) {
        printf("Invalid filename length.\n");
        snprintf(buff, SIZE_BUF, "Invalid filename length. Canceling.\n");
        send(clnt, buff, strlen(buff), 0);
        return;
    }

    // Проверка существования нужного файла в директории пользователя
    snprintf(buff, SIZE_BUF, "%s/%s", FOLDER_FILES, login[auth_user]);
    buff[strlen(buff)] = '\0';
    
    DIR* dir = opendir(buff);
    if (!dir) {
        perror("diropen");
    }
    struct dirent* flnm;
    while ((flnm = readdir(dir)) != NULL) {
        // Если файл найден
        if (strlen(flnm->d_name) > 2 && !strncmp(flnm->d_name, filename, sizeof(filename))) {
            printf("A valid filename has been received. Start deleting a file.\n");
            break;
        }
    }
    // Если файл не найден
    if (flnm == NULL){
        printf("The specified file was not found in the user's directory.\n");
        snprintf(buff, SIZE_BUF, "File \'%s\' is not in your directory.\n", filename);
        send(clnt, buff, strlen(buff), 0);
        return;
    }
    closedir(dir);

    // Создание пути до нужного файла
    int templen = strlen(FOLDER_FILES) + LEN_LOGPASS + strlen(filename) + 2;
    char tempdir[templen];

    snprintf(tempdir, templen, "%s/%s/%s", FOLDER_FILES, login[auth_user], filename);
    tempdir[templen] = '\0';
    
    if (remove(tempdir)) {
        printf("File deletion error.\n");
        snprintf(buff, SIZE_BUF, "File deletion error \'%s\'.\n", tempdir);
        send(clnt, buff, strlen(buff), 0);
        return;
    }

    printf("Successfully deleting the file \'%s\'.\n", tempdir);
    snprintf(buff, SIZE_BUF, "Successfully deleting the file \'%s\'.\n", tempdir);
    send(clnt, buff, strlen(buff), 0);
}

// Получение файла клиент -> сервер
void sendFileCom(SOCKET clnt, char* buff, char login[][LEN_LOGPASS], int auth_user) {
    // Проверка на вход/символы/корректность команды
    if (checkFilename(clnt, buff, "-sendfile", auth_user)) {
        return;
    }

    // Выборка названия файла из вызова команды
    char filename[strlen(buff) - sizeof("-sendfile") - getQtyChar(buff, ' ') + 2];
    int j = 0;
    for (int i = sizeof("-sendfile"); i < strlen(buff); i++) {
        if (buff[i] == ' ') { continue; }
        filename[j++] = buff[i];
    }
    filename[j] = '\0';
    printf("[DB] Filename: \'%s\' fnlen%d fnsz%d buflen%d.\n", filename, strlen(filename), sizeof(filename), strlen(buff));

    // Проверка длины
    if (sizeof(filename) > LEN_FILENAME || sizeof(filename) < 2) {
        printf("Invalid filename length.\n");
        snprintf(buff, SIZE_BUF, "Invalid filename length. Canceling.\n");
        send(clnt, buff, strlen(buff), 0);
        return;
    }

    // Проверка существования такого файла
    snprintf(buff, SIZE_BUF, "%s/%s", FOLDER_FILES, login[auth_user]);
    buff[strlen(buff)] = '\0';
    
    DIR* dir = opendir(buff);
    if (!dir) {
        perror("diropen");
    }
    struct dirent* flnm;
    while ((flnm = readdir(dir)) != NULL) {
        // Если файл найден
        if (strlen(flnm->d_name) > 2 && !strncmp(flnm->d_name, filename, sizeof(filename))) {
            printf("A file with this name already exists in the user's directory.\n");
            snprintf(buff, SIZE_BUF, "A file with this name already exists in the user's directory.\n", filename);
            send(clnt, buff, strlen(buff), 0);
            return;
        }
    }
    // Если файл не найден
    if (flnm == NULL){
        printf("The file was successfully not found in the user's directory.\n");
    }
    closedir(dir);

    // Создание пути до нужного файла
    int templen = strlen(FOLDER_FILES) + LEN_LOGPASS + strlen(filename) + 2;
    char tempdir[templen];

    snprintf(tempdir, templen, "%s/%s/%s", FOLDER_FILES, login[auth_user], filename);
    tempdir[templen] = '\0';
    printf("Opening a file \'%s\' %d for writing.\n", tempdir, templen);

    // Отправка команды и названия файла
    snprintf(buff, SIZE_BUF, "SEND %s", filename);
    send(clnt, buff, strlen(buff), 0);

    // Получение пароля для регистрации
    if (recv(clnt, buff, SIZE_BUF, 0) == SOCKET_ERROR) {
        printf("Error getting password.\n");
        closesocket(clnt);
        exit(0);
    }

    if (!strncmp(buff, "-END", 4)) {
        printf("The file upload was canceled by the client.\n");
        snprintf(buff, SIZE_BUF, "The file upload was canceled by the client.\n");
        send(clnt, buff, strlen(buff), 0);
        return;
    }

    FILE *file = fopen(tempdir, "wb");
    if (file == NULL) {
        printf("Creating file error.\n");
        fclose(file);
        closesocket(clnt);
        exit(0);
    }

    // Принятие и сохранение файла
    int len;
    while (1) {
        if ((len = recv(clnt, buff, SIZE_BUF, 0)) == SOCKET_ERROR) {
            printf("File retrieval error.\n");
            fclose(file);
            closesocket(clnt);
            exit(0);
        }
        if (!strncmp(buff, "-END", 4)) {
            break;
        }
        if (len > 0) {
            fwrite(buff, 1, len, file);
        } 
    }
    fclose(file);

    printf("Successfully receiving and saving of the file \'%s\'!\n", filename);
    snprintf(buff, SIZE_BUF, "Successfully receiving and saving of the file \'%s\'.\n", tempdir);
    send(clnt, buff, strlen(buff), 0);
}

// Отправка файла сервер -> клиент
void getFileCom(SOCKET clnt, char* buff, char login[][LEN_LOGPASS], int auth_user) {
    // Проверка на вход/символы/корректность команды
    if (checkFilename(clnt, buff, "-getfile", auth_user)) {
        return;
    }

    // Выборка названия файла из вызова команды
    char filename[strlen(buff) - sizeof("-getfile") - getQtyChar(buff, ' ') + 2];
    int j = 0;
    for (int i = sizeof("-getfile"); i < strlen(buff); i++) {
        if (buff[i] == ' ') { continue; }
        filename[j++] = buff[i];
    }
    filename[j] = '\0';
    printf("[DB] Filename: \'%s\' fnlen%d fnsz%d buflen%d.\n", filename, strlen(filename), sizeof(filename), strlen(buff));

    // Проверка длины
    if (sizeof(filename) > LEN_FILENAME || sizeof(filename) < 2) {
        printf("Invalid filename length.\n");
        snprintf(buff, SIZE_BUF, "Invalid filename length. Canceling.\n");
        send(clnt, buff, strlen(buff), 0);
        return;
    }

    // Проверка существования нужного файла в директории пользователя
    snprintf(buff, SIZE_BUF, "%s/%s", FOLDER_FILES, login[auth_user]);
    buff[strlen(buff)] = '\0';
    
    DIR* dir = opendir(buff);
    if (!dir) {
        perror("diropen");
    }
    struct dirent* flnm;
    while ((flnm = readdir(dir)) != NULL) {
        // Если файл найден
        if (strlen(flnm->d_name) > 2 && !strncmp(flnm->d_name, filename, sizeof(filename))) {
            printf("A valid filename has been received. Start of file transfer.\n");
            break;
        }
    }
    // Если файл не найден
    if (flnm == NULL){
        printf("The specified file was not found in the user's directory.\n");
        snprintf(buff, SIZE_BUF, "File \'%s\' is not in your directory.\n", filename);
        send(clnt, buff, strlen(buff), 0);
        return;
    }
    closedir(dir);

    // Создание пути до нужного файла
    int templen = strlen(FOLDER_FILES) + LEN_LOGPASS + strlen(filename) + 2;
    char tempdir[templen];

    snprintf(tempdir, templen, "%s/%s/%s", FOLDER_FILES, login[auth_user], filename);
    tempdir[templen] = '\0';
    printf("Opening a file \'%s\' %d for reading.\n", tempdir, templen);

    // Отправка команды и названия файла
    snprintf(buff, SIZE_BUF, "GET %s", filename);
    send(clnt, buff, strlen(buff), 0);

    FILE *file = fopen(tempdir, "rb");
    if (file == NULL) {
        printf("Failed to open file for reading.\n");
        closesocket(clnt);
        exit(0);
    }

    // Отправка файла
    while (1) {
        int bytesRead = fread(buff, 1, SIZE_BUF, file);
        if (bytesRead > 0) {
            send(clnt, buff, bytesRead, 0);
        } else {
            break;
        }
    }
    fclose(file);

    Sleep((DWORD)30);
    // Отправка сообщения об окончании передачи файла
    snprintf(buff, SIZE_BUF, "-END");
    send(clnt, buff, strlen(buff), 0);

    printf("Successful acceptance and saving of the file \'%s\'.\n", tempdir);
    snprintf(buff, SIZE_BUF, "Successful acceptance and saving of the file \'%s\'.\n", tempdir);
    send(clnt, buff, strlen(buff), 0);
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

// Создание системных и файловых каталогов, выгрузка passlog файлов
int createDir(char login[][LEN_LOGPASS], char password[][LEN_LOGPASS]) {
    int log_counter = 0;
    char tempdir[128];

    // Создание logpass каталога
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
    if (listen(s, 10) == SOCKET_ERROR) {
        printf("Error while listening for incoming connections.\n");
        closesocket(s);
        exit(0);
    }
    return s;
}
