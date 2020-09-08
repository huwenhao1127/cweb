#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

// logback值为监听队列的上限
int main(int argc, char* argv[]) 
{   
    if(argc <= 2) {
        printf("arg < 3");
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);
    int backlog = 5;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr);

    int ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sock, backlog);
    assert(ret != -1);

    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);

    int connfd = accept(sock, (struct sockaddr*)&client, &client_len);
    if (connfd < 0) {
        std::cout << "accept error" << std::endl;
    } else {
        close(STDOUT_FILENO);   // 关闭本地的标准输出文件描述符
        dup(connfd);
        std::cout << "abcd" << std::endl;
        close(connfd);
    }

    close(sock);
    return 0;
}