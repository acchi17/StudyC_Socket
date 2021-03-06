#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define RECV_MSG_SIZE 2048

char recvMsg[RECV_MSG_SIZE];

int main()
{
    int serverSock;
    int clientSock;
    int size_clientAddr;
    int size_recvByte;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;

    // ソケットを作成
    serverSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    printf("Server socket discripter: %d\n", serverSock);

    // サーバのIPアドレスとポート番号を指定
    memset(&serverAddr, 0, sizeof(serverAddr));           // 0で初期化しないと不具合が出るらしい
    serverAddr.sin_family      = AF_INET;                 // アドレスの種類:IPv4を設定
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // 現在のIPアドレスを設定
    serverAddr.sin_port        = htons(26262);            // ポート番号を設定

    // ソケットにIPアドレスとポート番号を設定(Bind)
    bind(serverSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    // ソケットを接続可能状態にする(Listen)
    listen(serverSock, 1);

    while(true)
    {
        printf("Listening...\n");
        size_clientAddr = sizeof(clientAddr);
        // クライアントからの接続を受け付ける(Accept)
        clientSock = accept(serverSock, (struct sockaddr *)&clientAddr, &size_clientAddr); // ブロッキング
        printf("Client connected! IP address: %s\n", inet_ntoa(clientAddr.sin_addr));

        while(true)
        {
            memset(recvMsg, 0, sizeof(recvMsg));
            // クライアントから受信
            size_recvByte = recv(clientSock, recvMsg, sizeof(recvMsg), 0); // ブロッキング
            if (size_recvByte <= 0)
            {
                printf("Client disconnected! IP address: %s\n", inet_ntoa(clientAddr.sin_addr));
                close(clientSock);
                break;
            }
            else
            {
                printf("Receive: %s\n", recvMsg);
                // クライアントへ送信
                send(clientSock, recvMsg, size_recvByte, 0);
                printf("Send: %s\n", recvMsg);
            }
        }
    }

    return 0;
}