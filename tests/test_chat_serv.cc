#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <errno.h>
#include <fcntl.h>

#define BUFFER_SIZE 64
#define USER_LIMIT 5
#define FD_LIMIT 65535

// 客户数据: 客户端socket地址、待写到客户端的数据的位置、从客户端读入的数据
struct client_data
{
    sockaddr_in address;
    char* write_buf;
    char buf[BUFFER_SIZE];
};

int setNonblocking(int fd)
{
    int old_op = fcntl(fd, F_GETFL);
    int new_op = old_op | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_op);
    return old_op;
}

int main(int argc, char* argv[])
{  
    if(argc <= 2) {
        printf("arg error");
        return 1;
    }

    char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr);
    int ret = 0;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);

    ret = bind(sockfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sockfd, 5);
    assert(ret != -1);

    /* 创建users数组, 分配FD_LIMIT个client_data对象, 可以预期, 每个可能的socket链接都可以获得一个这样的对象,
    并且socket的值可以直接用来索引socket链接对应的client_data对象, 这是将socket和客户数据关联的简单有效的方式 */
    client_data* users = new client_data[FD_LIMIT];

    pollfd fds[USER_LIMIT+1];
    int user_counter = 0;

    for(int i = 1; i <= USER_LIMIT; ++i) {
        fds[i].fd = -1;
        fds[i].events = 0;
    } 
    fds[0].fd = sockfd;
    fds[0].events = POLLIN | POLLERR;
    fds[0].revents = 0;

    while(1) {
        ret = poll(fds, user_counter+1, -1);
        if( ret < 0) {
            printf("poll failure\n");
            break;
        }

        for(int i=0; i < user_counter+1; ++i) {
            if((fds[i].fd == sockfd) && (fds[i].revents & POLLIN)) {
                struct sockaddr_in client;
                socklen_t len = sizeof(client);
                int connfd = accept(sockfd, (struct sockaddr*)& client, &len);
                if(connfd < 0) {
                    printf("error is: %d\n", errno);
                    continue;
                }

                // 如果请求太多,则关闭新到的连接
                if( user_counter >= USER_LIMIT) {
                    const char* info = "too many users\n";
                    printf("%s", info);
                    send(connfd, info, strlen(info), 0);
                    close(connfd);
                    continue;
                }

                // 对于新到的链接, 同时修改fds和users数组
                user_counter++;
                users[connfd].address = client;
                setNonblocking(connfd);
                fds[user_counter].fd = connfd;
                fds[user_counter].events = POLLIN | POLLRDHUP | POLLERR;
                fds[user_counter].revents = 0;
                printf("comes a new user, now have %d users\n", user_counter);
            } else if(fds[i].revents & POLLERR) {
                printf("get an error from %d\n", fds[i].fd);
                char errors[100];
                memset(errors, '\0', sizeof(errors));
                socklen_t length = sizeof(errors);
                if(getsockopt(fds[i].fd, SOL_SOCKET, SO_ERROR, &errors, &length) < 0) {
                    printf("get socket opt failed\n");
                }
                continue;

            } else if(fds[i].revents & POLLRDHUP) {
                // 客户端关闭链接,服务器也关闭对应链接
                users[fds[i].fd] = users[fds[user_counter].fd];
                close(fds[i].fd);
                fds[i] = fds[user_counter];
                --i;
                --user_counter;
                printf("a client left");
            } else if(fds[i].revents & POLLIN) {
                int connfd = fds[i].fd;
                memset(users[connfd].buf, '\0', BUFFER_SIZE);
                ret = recv(connfd, users[connfd].buf, BUFFER_SIZE-1, 0);
                printf("%d : %s [%d bytes]\n", connfd, users[connfd].buf, ret);
                if( ret < 0) {
                    if (errno != EAGAIN) {
                        close(connfd);
                        users[fds[i].fd] = users[fds[user_counter].fd];
                        fds[i] = fds[user_counter];
                        --i;
                        --user_counter;
                    }
                } else if(ret == 0) {
                    
                } else {
                    // 如果接收到客户数据,则通知其他socket链接准备写数据
                    for(int j = 1; j <= user_counter; ++j) {
                        if(fds[j].fd == connfd) {
                            continue;
                        }
                        fds[j].events |= ~POLLIN;
                        fds[j].events |= POLLOUT;
                        users[fds[j].fd].write_buf = users[connfd].buf;
                    }
                }
            } else if(fds[i].revents & POLLOUT) {
                int connfd = fds[i].fd;
                if(!users[connfd].write_buf ) {
                    continue;
                } 
                ret = send(connfd, users[connfd].write_buf, strlen(users[connfd].write_buf), 0);
                users[connfd].write_buf = NULL;
                // 重新注册fds[i]上的可读事件
                fds[i].events |= ~POLLOUT;
                fds[i].events |= POLLIN;

            }
        }
    }
    delete [] users;
    close(sockfd);
    return 0;

}