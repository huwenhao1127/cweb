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

int main(int argc, char* argv[]) 
{   
    if(argc <= 2) {
        printf("arg < 2");
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr);

    if(connect(sock, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cout << "error" <<std::endl;
    } else {
        const char* oob_data = "abc";
        const char* nor_data = "123";
        send(sock, nor_data, strlen(nor_data), 0);
        send(sock, oob_data, strlen(oob_data), MSG_OOB);
        send(sock, nor_data, strlen(nor_data), 0);
    }

    close(sock);
    return 0;
}