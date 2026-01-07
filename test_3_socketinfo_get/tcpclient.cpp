#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./tcpSocketInfoTool.h"

#define PORT 8080

int main() {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }
  
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
      
    // 将地址从字符串转换为AF_INET类型
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)  {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
  
    // 连接到服务端
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    int cnt = 0;
    while(1){
        sleep(1);

        // 发送消息到服务端
        char *hello = "Hello from client";
        send(sock, hello, strlen(hello), 0);
        printf("<%d>Message to server: %s\n", cnt++, hello);
        
        // 读取服务端响应
        valread = read(sock, buffer, 1024);
        printf("<%d>Message from server: %s\n", cnt++, buffer);

        printf("-----------------------\n");

        tcpSocketInfoTool tcp_info(sock);
        tcp_info.dump();

        printf("======================\n");
        printf("\n");
    }
    
    // 关闭socket
    close(sock);
    return 0;
}
