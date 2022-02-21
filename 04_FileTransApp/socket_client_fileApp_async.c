#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

#define SEND_MSG_SIZE 8388608
#define RECV_MSG_SIZE 8388608
#define RECV_BUF_SIZE 1048576
#define TEXT_FIELD_SIZE 256
#define TEXT_ITEM_NUM 2
#define DELIMITER ";"

char sendMsg[SEND_MSG_SIZE];
char recvMsg[RECV_MSG_SIZE];
char recvBuf[RECV_BUF_SIZE];
char recvNtfyMsg[TEXT_FIELD_SIZE*2];
char *cmd_inf = "Inferrence";
char *rcmd_inf_n = "R_Inferrence_N";
char *rcmd_inf_e = "R_Inferrence_E";

void setFront4byteData(char *dst, int data)
{
    // Example:
    // data = 0x41424344 -> dst[0]:0x44, dst[1]:0x43, dst[2]:0x42, dst[3]:0x41,
    dst[0] = (char)(data); 
    dst[1] = (char)(data >> 8);
    dst[2] = (char)(data >> 16);
    dst[3] = (char)(data >> 24);
}

void getFileName(char *fileName, char *filePath, int size_max)
{
    char *p_deli;
    int size_fileName;

    if (strlen(filePath) > 0)
    {
        p_deli = strrchr(filePath, (int)'/');
        if (p_deli != NULL)
        {
            p_deli = p_deli + 1;
            size_fileName = strlen(p_deli);
            if (size_fileName < size_max)
            {
                // strncpyが最後尾にnullを入れるようにするため
                // size_fileName+1とする
                strncpy(fileName, p_deli, size_fileName+1);
            }  
        }
    }   
}

// 入力： src, delim
//   src:   入力文字列
//   delim: デリミタ文字
//   size_dst dstの要素数
// 出力：dst, return
//   dst:    デリミタ文字で区切られた文字列のリスト
//   return: デリミタ文字で区切られた要素数
//
// 例：
// src: abcd/efgh/ijkl\0
// delim: /
// size_dst: 2
// ->
// dst[0]: abcd\0, dst[1]: efgh\0, 
// return: 
int split(char *dst[], char *src, char delim, int size_dst)
{
  int count = 0;

  if (*src != '\0')
  {
      while(1)
      {
          count++;
          dst[count-1] = src;
          // デリミタ文字を探索
          while ((*src != '\0') && !(*src == delim)) { src++; } 
          // null文字を見つけるまたはdst要素数以上になったら終了
         if (*src == '\0') break;      
         *src = '\0';
         if (count >= size_dst) break;
         src++;
      }
  }

  return count;
}

void * recv_thread(void *arg)
{
    int size_recvByte;
    int clientSock = *((int *)arg);
    char recvText[TEXT_FIELD_SIZE];
    char filePath[TEXT_FIELD_SIZE];
    char *textList[TEXT_ITEM_NUM];
    int size_recvByteTotal;
    int size_recvByteTotalExp;
    int size_writeByte;
    int saveFile;

    while(true)
    {
        // サーバから受信
        memset(recvBuf, 0, sizeof(recvBuf));
        size_recvByte = recv(clientSock, recvBuf, sizeof(recvBuf), 0);
        if (size_recvByte > 0)
        {
            //printf("Receive size: %d\n", size_recvByte);
            // 受信予定総バイト数取得処理
            if (size_recvByteTotalExp <= 0)
            {
                size_recvByteTotalExp = *((int*)recvBuf);
                printf("Expected total receive size: %d\n", size_recvByteTotalExp);
                if ((size_recvByteTotalExp > RECV_MSG_SIZE) || (size_recvByteTotalExp < 0))
                {
                    size_recvByteTotalExp = 0;
                    printf("Expected total receive size is too many or invalid!\n");
                }
                else
                {
                    memset(recvMsg, 0, sizeof(recvMsg));
                    memset(recvText, 0, sizeof(recvText));
                }
            }
            // 受信データ処理
            if (size_recvByteTotalExp > 0)
            {
                // 受信バイトデータを保存
                memcpy(recvMsg+size_recvByteTotal, recvBuf, size_recvByte);
                // 受信済総バイト数を計算
                size_recvByteTotal = size_recvByteTotal + size_recvByte;
                // 受信データをすべて受信した後の処理 //
                if (size_recvByteTotal >= size_recvByteTotalExp)
                {
                    sprintf(recvNtfyMsg, "Response message is unknown...\n");
                    memset(recvText, 0, sizeof(recvText));
                    // 受信データからテキストデータを取得
                    memcpy(recvText, recvMsg+4, TEXT_FIELD_SIZE);
                    if (split(textList, recvText, DELIMITER[0], TEXT_ITEM_NUM) >= TEXT_ITEM_NUM)
                    {
                        // 応答：処理成功の処理
                        if (textList[0] == rcmd_inf_n)
                        {
                            // 保存ファイルパス作成
                            memset(filePath, 0, sizeof(filePath));
                            strcat(filePath, "./");
                            strcat(filePath, textList[1]);
                            filePath[sizeof(filePath)-1] = '\0';
                            // 保存した受信データをファイルに保存
                            saveFile = creat(filePath, S_IREAD|S_IWRITE);
                            size_writeByte = 0;
                            if (saveFile > 0)
                            {
                                // 受信データをファイルに書込
                                size_writeByte = write(saveFile, recvMsg+4+TEXT_FIELD_SIZE, size_recvByteTotalExp-4-TEXT_FIELD_SIZE);
                            }
                            if (size_writeByte > 0)
                            {
                                // ファイル書込成功時の処理
                                sprintf(recvNtfyMsg, "File saved successfully! File path: %s\n", filePath);
                            }
                            else
                            {
                                // ファイル書込失敗時の処理
                                sprintf(recvNtfyMsg, "Error occurred while whiting specified file!\n");
                            }
                            close(saveFile);
                        }
                        // 応答：処理失敗の処理
                        else if (textList[0] == rcmd_inf_e)
                        {
                            // エラーメッセージを格納
                            sprintf(recvNtfyMsg, textList[1]);
                        }
                    }
                    size_recvByteTotalExp = 0;
                    size_recvByteTotal = 0;    
                }
            }
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
    char filePath[TEXT_FIELD_SIZE];
    char fileName[TEXT_FIELD_SIZE];
    char textField[TEXT_FIELD_SIZE];

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
        memset(recvNtfyMsg, 0, sizeof(recvNtfyMsg));
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
                    memset(fileName, 0, sizeof(fileName));
                    // ファイルパスからファイル名を取得
                    getFileName(fileName, filePath, sizeof(fileName));
                    printf("Text field size:%d\n", (strlen(cmd_inf) + strlen(DELIMITER) + strlen(fileName)));
                    if ((strlen(cmd_inf) + strlen(DELIMITER) + strlen(fileName)) <= TEXT_FIELD_SIZE )
                    {
                        memset(textField, 0, sizeof(textField));
                        memset(sendMsg, 0, sizeof(sendMsg));
                        // テキストフィールドにコマンド文字列、デリミタ、ファイル名を格納する
                        strcat(textField, cmd_inf);
                        strcat(textField, DELIMITER);
                        strcat(textField, fileName);
                        printf("Text field string:%s\n", textField);
                        // 送信データの5バイト目以降にテキストフィールドを格納する
                        strncpy(sendMsg+4, textField, TEXT_FIELD_SIZE);
                        // ファイルの中身を送信データの261バイト目以降に格納する
                        size_sendMsg = read(clientFile, sendMsg+4+TEXT_FIELD_SIZE, sizeof(sendMsg)-4-TEXT_FIELD_SIZE);
                        printf("Read file size:%d\n", size_sendMsg);
                        size_sendMsg = size_sendMsg + 4 + TEXT_FIELD_SIZE;
                        // 送信データの先頭4バイトに送信サイズを格納する
                        setFront4byteData(sendMsg, size_sendMsg);
                        printf("Send message size:%d\n", *((int*)sendMsg));
                        memset(recvNtfyMsg, 0, sizeof(recvNtfyMsg));
                        // サーバへ送信
                        send(clientSock, sendMsg, size_sendMsg, 0);
                    }
                    else
                    {
                        printf("Error, file size is too big!\n");
                    }                    
                }
                else
                {
                    printf("Error occurred while reading specified file!\n");
                }
                close(clientFile);
            }
            else if (strcmp(inputBuf, "1") == 0)
            {
                printf("Receive status: %s\n", recvNtfyMsg);
            }
        }
        // サーバから切断
        close(clientSock);
        pthread_join( tid, NULL );
    }

    return 0;
}