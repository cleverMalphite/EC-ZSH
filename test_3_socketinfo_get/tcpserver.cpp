#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "./tcpSocketInfoTool.h"

#define PORT 8080

int main() {
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // 创建socket文件描述符
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // 绑定socket地址
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
      
    // 强制绑定socket到8080端口
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // 监听socket
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // 接受客户端连接
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    int cnt = 0;
    while(1){

        // 读取客户端发送的消息
        char buffer[1024] = {0};
        valread = read(new_socket, buffer, 1024);
        printf("<%d>Message from client: %s\n", cnt++, buffer);

        char* resp = "Hello from server2";
        send(new_socket, resp, strlen(resp), 0);
        printf("<%d>respons to client: %s\n", cnt++, resp);

        printf("-----------------\n");

        tcpSocketInfoTool tcp_info(new_socket);
        tcp_info.dump();

        printf("======================\n");
        printf("\n");

    }
    
    // 关闭socket
    close(new_socket);
    close(server_fd);
    return 0;
}
