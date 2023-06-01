#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define SIZE_BUF 512
#define LEN_LOGPASS 11
#define QTY_LOGPASS 10
#define LEN_FILENAME 32

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

int procMsgr(char* buff);

void stopServ(SOCKET serv, SOCKET clnt);

int authUser(SOCKET clnt, char* buff, char pass[][LEN_LOGPASS], char login[][LEN_LOGPASS], int counter, int auth_user);

int regUser(SOCKET clnt, char* buff, char pass[][LEN_LOGPASS], char login[][LEN_LOGPASS], int counter, int auth_user);

void showList(SOCKET clnt, char* buff, char login[][LEN_LOGPASS], int auth_user);

void removeFileCom(SOCKET clnt, char* buff, char login[][LEN_LOGPASS], int auth_user);

void sendFileCom(SOCKET clnt, char* buff, char login[][LEN_LOGPASS], int auth_user);

void getFileCom(SOCKET clnt, char* buff, char login[][LEN_LOGPASS], int auth_user);

int createDir(char login[][LEN_LOGPASS], char password[][LEN_LOGPASS]);

char* getSockIp(SOCKET s);

SOCKET getHost(char* ipAddr, int port);