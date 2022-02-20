#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SEND_MSG_SIZE 2048
#define RECV_MSG_SIZE 2048

char sendMsg[SEND_MSG_SIZE];
char recvMsg[RECV_MSG_SIZE];

int main()
{
    int res;
    int clientSock;
    int size_sendMsg;
    struct sockaddr_in serverAddr;

    // ソケットを作成
    clientSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    printf("Client socket discripter: %d\n", clientSock);

    // サーバのIPアドレスとポート番号を指定
    memset(&serverAddr, 0, sizeof(serverAddr));           // 0で初期化しないと不具合が出るらしい
    serverAddr.sin_family      = AF_INET;                 // アドレスの種類:IPv4を設定
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // 接続先IPアドレスを設定
    serverAddr.sin_port        = htons(26262);            // ポート番号を設定

    // サーバに接続
    res = connect(clientSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)); // 接続が成功すると0を返す
    if (res == 0)
    {
        printf("Connected!\n");
        while(true)
        { 
            // 入力待ち
            printf("Input send message >>");
            gets(sendMsg);
            // 入力チェック
            if (strcmp(sendMsg, "exit") == 0)
            {
                break;
            }
            // サーバへ送信
            size_sendMsg = strlen(sendMsg);
            send(clientSock, sendMsg, size_sendMsg, 0);
            printf("Send: %s\n", sendMsg);
            // サーバから受信
            recv(clientSock, recvMsg, sizeof(recvMsg), 0); // ブロッキング
            printf("Receive: %s\n", recvMsg);
        }
        // サーバから切断
        close(clientSock);
    }

    return 0;
}