#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <assert.h>
#include <iostream>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

// 要先开启daytime服务

int main(int argc, char* argv[]) 
{   
    if(argc <= 2) {
        printf("arg < 2");
        return 1;
    }

    const char* host = argv[1];
    //int port = atoi(argv[2]);

    // 获取主机地址信息
    struct hostent* hostinfo = gethostbyname(host);
    assert(hostinfo);
    // 获取daytime服务信息
    struct servent* servinfo = getservbyname("daytime", "tcp");
    assert(servinfo);
    std::cout << "daytime port is : " << servinfo->s_port << std::endl;
    std::cout << "host addr is : " << hostinfo->h_addr_list << std::endl;


    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = servinfo->s_port;
    address.sin_addr = *(struct in_addr*)*hostinfo->h_addr_list;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    int result = connect(sock, (struct sockaddr*)&address, sizeof(address));
    assert(result != -1);

    char buf[128];
    result = read(sock, buf, sizeof(buf));
    assert(result > 0);
    buf[result] = '\0';

    std::cout << "the daytime : " << buf << std::endl;
    
    close(sock);
    return 0;
}