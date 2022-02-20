#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>

#define SEND_MSG_SIZE 2048
#define RECV_MSG_SIZE 2048

char sendMsg[SEND_MSG_SIZE];
char recvMsg[RECV_MSG_SIZE];

int split(char *dst[], char *src, char delim)
{
  int count = 0;

  if (*src == '\0') return count;

  while(1)
  {
      dst[count] = src;
      count++;
      while ((*src != '\0') && !(*src == delim)) { src++; } // デリミタ文字を探索
      if (*src == '\0') break;                              // null文字を見つけたら終了
      *src = '\0';
      src++;
  }

  return count;
}

//
// Server resposes each command
//  Command pattern:
//    "Add n1 n2": response n1+n2
//    "Sub n1 n2": response n1-n2
//
int main()
{
    int serverSock;
    int clientSock;
    int size_clientAddr;
    int size_recvByte;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    char *cmdList[100];
    int resNum;

    // サーバ用ソケットの作成
    serverSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    printf("Server socket discripter: %d\n", serverSock);

    // サーバのIPアドレスとポート番号設定
    memset(&serverAddr, 0, sizeof(serverAddr));           // 0で初期化しないと不具合が出るらしい
    serverAddr.sin_family      = AF_INET;                 // アドレスの種類:IPv4を設定
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // 現在のIPアドレスを設定
    serverAddr.sin_port        = htons(26262);            // ポート番号を設定

    // バインド
    bind(serverSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    // リスン
    listen(serverSock, 1); // クライアントからの接続を受け付ける

    while(true)
    {
        printf("Listening...\n");
        size_clientAddr = sizeof(clientAddr);
        // クライアントからの接続を待つ(ブロッキング)
        clientSock = accept(serverSock, (struct sockaddr *)&clientAddr, &size_clientAddr);
        printf("Client connected! IP address: %s\n", inet_ntoa(clientAddr.sin_addr));

        while(true)
        {
            memset(recvMsg, 0, sizeof(recvMsg));
            // クライアントから受信(ブロッキング)
            size_recvByte = recv(clientSock, recvMsg, sizeof(recvMsg), 0);
            if (size_recvByte <= 0)
            {
                printf("Client disconnected! IP address: %s\n", inet_ntoa(clientAddr.sin_addr));
                close(clientSock);
                break;
            }
            else
            {
                printf("Receive: %s\n", recvMsg);
                memset(sendMsg, 0, sizeof(sendMsg));
                sleep(5); // 5秒停止
                if (split(cmdList, recvMsg, ' ') >= 3)
                {
                    resNum = 0;
                    if      (cmdList[0] == "Add") resNum = atoi(cmdList[1]) + atoi(cmdList[2]);
                    else if (cmdList[0] == "Sub") resNum = atoi(cmdList[1]) - atoi(cmdList[2]);
                    sprintf(sendMsg, "Result: %d", resNum);
                }
                else
                {
                    strcpy(sendMsg, "Unknown command");
                }
                printf("Send: %s\n", sendMsg);
                // クライアントに返信
                send(clientSock, sendMsg, strlen(sendMsg), 0);
            }
        }
    }

    return 0;
}