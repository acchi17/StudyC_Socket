#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

#define SEND_MSG_SIZE 8388608
#define RECV_MSG_SIZE 2048

char sendMsg[SEND_MSG_SIZE];
char recvMsg[RECV_MSG_SIZE];

void setFront4byteData(char *dst, int data)
{
    // Example:
    // data = 0x41424344 -> dst[0]:0x44, dst[1]:0x43, dst[2]:0x42, dst[3]:0x41,
    dst[0] = (char)(data); 
    dst[1] = (char)(data >> 8);
    dst[2] = (char)(data >> 16);
    dst[3] = (char)(data >> 24);
}

void * recv_thread(void *arg)
{
    int size_recvByte;
    int clientSock = *((int *)arg);
    char recvBuf[RECV_MSG_SIZE];

    while(true)
    {
        // サーバから受信
        memset(recvBuf, 0, sizeof(recvBuf));
        size_recvByte = recv(clientSock, recvBuf, sizeof(recvBuf), 0);
        if (size_recvByte > 0)
        {
            //printf("Thread recv size:%d\n", size_recvByte);
            if (size_recvByte > RECV_MSG_SIZE) { size_recvByte = RECV_MSG_SIZE; } 
            strncpy(recvMsg, recvBuf, size_recvByte);
            recvMsg[size_recvByte] = '\0';
        }
        else
        {
            break;
        }
    }
}

int main()
{
    int res;
    int clientSock;
    int size_sendMsg;
    struct sockaddr_in serverAddr;
    char inputBuf[10];
    pthread_t tid;
    int clientFile;
    char filePath[512];

    // クライアント用ソケットの作成
    clientSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    printf("Client socket discripter: %d\n", clientSock);

    // サーバのIPアドレスとポート番号設定
    memset(&serverAddr, 0, sizeof(serverAddr));           // 0で初期化しないと不具合が出るらしい
    serverAddr.sin_family      = AF_INET;                 // アドレスの種類:IPv4を設定
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // 接続先IPアドレスを設定
    serverAddr.sin_port        = htons(26262);            // ポート番号を設定

    // サーバに接続
    res = connect(clientSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)); // 接続が成功すると0を返す
    if (res == 0)
    {
        printf("Connected!\n");
        pthread_create(&tid, NULL, recv_thread, &clientSock);
        while(true)
        { 
            // 入力待ち
            printf("\n0: Send file / 1: Show received message >>");
            gets(inputBuf);
            // 入力チェック
            if (strcmp(inputBuf, "exit") == 0)
            {
                break;
            }
            else if (strcmp(inputBuf, "0") == 0)
            {
            	memset(filePath, 0, sizeof(filePath));
                printf("Input send file path >>");
                gets(filePath);
                // ファイルパスにあるファイルを開く
                printf("Input: %s\n", filePath);
                clientFile = open(filePath, O_RDONLY);
                if (clientFile > 0)
                {
                    memset(sendMsg, 0, sizeof(sendMsg));
                    size_sendMsg = read(clientFile, sendMsg+4, sizeof(sendMsg));
                    printf("Read file size:%d\n", size_sendMsg);
                    // 送信データの先頭4バイトに送信サイズを格納する
                    size_sendMsg = size_sendMsg + 4;
                    setFront4byteData(sendMsg, size_sendMsg);
                    printf("Send message size:%d\n", *((int*)sendMsg));
                    // サーバへ送信
                    send(clientSock, sendMsg, size_sendMsg, 0);
                }
                else
                {
                    printf("Error occurred while reading specified file!\n");
                }
                close(clientFile);
            }
            else if (strcmp(inputBuf, "1") == 0)
            {
                printf("Receive message: %s\n", recvMsg);
            }
        }
        // サーバから切断
        close(clientSock);
        pthread_join( tid, NULL );
    }

    return 0;
}