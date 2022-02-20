#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define SEND_MSG_SIZE 2048
#define RECV_MSG_SIZE 1048576
#define FILE_BUF_SIZE 8388608

char sendMsg[SEND_MSG_SIZE];
char recvMsg[RECV_MSG_SIZE];
char fileBuf[FILE_BUF_SIZE];

int main()
{
    int serverSock;
    int clientSock;
    int size_clientAddr;
    int size_recvByte;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    int size_recvByteTotal;
    int size_recvByteTotalExp;
    int size_writeByte;
    int saveFile;

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
        size_recvByteTotalExp = 0;
        size_recvByteTotal = 0;
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
                printf("Receive size: %d\n", size_recvByte);
                if (size_recvByteTotalExp <= 0)
                {
                    // 受信予定の総バイト数を取得
                    size_recvByteTotalExp = *((int*)recvMsg);
                    printf("Expected total receive size: %d\n", size_recvByteTotalExp);
                    if ((size_recvByteTotalExp > FILE_BUF_SIZE) || (size_recvByteTotalExp < 0))
                    {
                        size_recvByteTotalExp = 0;
                        printf("Expected total receive size is too many or invalid!\n");
                    }
                    else
                    {
                        memset(fileBuf, 0, sizeof(fileBuf));
                    }
                }

                if (size_recvByteTotalExp > 0)
                {
                    // 受信バイトデータを保存
                    memcpy(fileBuf+size_recvByteTotal, recvMsg, size_recvByte);
                    // 受信済総バイト数を計算
                    size_recvByteTotal = size_recvByteTotal + size_recvByte;
                    if (size_recvByteTotal >= size_recvByteTotalExp)
                    {
                        // 保存した受信バイトデータをファイルに保存
                        saveFile = creat("./recvFile.png", S_IREAD|S_IWRITE);
                        size_writeByte = 0;
                        if (saveFile > 0)
                        {
                            size_writeByte = write(saveFile, fileBuf+4, size_recvByteTotalExp-4);
                        }
                        memset(sendMsg, 0, sizeof(sendMsg));
                        if (size_writeByte > 0)
                        {
                            sprintf(sendMsg, "File saved successfully! Saved file size: %d\n", size_writeByte);
                        }
                        else
                        {
                            sprintf(sendMsg, "Error occurred while whiting specified file!\n");
                        }
                        close(saveFile);
                        printf("Send: %s\n", sendMsg);
                        // クライアントに返信
                        send(clientSock, sendMsg, strlen(sendMsg), 0);
                        size_recvByteTotalExp = 0;
                        size_recvByteTotal = 0;
                    }
                }
            }
        }
    }

    return 0;
}