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
#include <errno.h>

#define BUF_SIZE 1024

// logback值为监听队列的上限
int main(int argc, char* argv[]) 
{   
    if(argc <= 2) {
        printf("arg < 2");
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
        std::cout << "accept error: " << errno;
    } else {
        char buffer[BUF_SIZE];

        memset(buffer, '\0', BUF_SIZE);
        ret  = recv(connfd, buffer, BUF_SIZE-1, 0);
        std::cout << "got " << ret << "bytes normal data: " 
                << buffer << std::endl;

        memset(buffer, '\0', BUF_SIZE);
        ret  = recv(connfd, buffer, BUF_SIZE-1, MSG_OOB);
        std::cout << "got " << ret << "bytes oob data: " 
                << buffer << std::endl;
        
        memset(buffer, '\0', BUF_SIZE);
        ret  = recv(connfd, buffer, BUF_SIZE-1, 0);
        std::cout << "got " << ret << "bytes normal data: " 
                << buffer << std::endl;

        close(connfd);
    }

    close(sock);
    return 0;
}