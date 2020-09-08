#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <pthread.h>

#define MAX_EVENT_NUM 1024
#define BUFFER_SIZE 1024

struct fds
{
    int epollfd;
    int sockfd;
};

// 设置文件描述符为非阻塞
int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

// 将文件描述符fd上的EPOLLIN注册到epollfd指示的epoll内核事件表中
void addfd(int epollfd, int fd, bool oneshot)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    if(oneshot) {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void reset_oneshot(int epollfd, int fd)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

// 工作线程
void* work(void* arg)
{
    int sockfd = ((fds*)arg)->sockfd;
    int epollfd = ((fds*)arg)->epollfd;
    printf("start new thread to receive data on fd: %d\n", sockfd);
    char buf[BUFFER_SIZE];
    memset(buf, '\0', BUFFER_SIZE);
    // 循环读socket上的数据,直到遇到EAGAIN
    while(1) {
        int ret = recv(sockfd, buf, BUFFER_SIZE-1, 0);
        if( ret == 0 ) {
            close(sockfd);
            printf("foreiner closed the connection\n");
            break;
        } else if(ret < 0) {
            if(errno == EAGAIN) {
                reset_oneshot(epollfd, sockfd);  // 只被触发一次,因此处理一次之后需要重置该socketfd
                printf("read later\n");
                break;
            }
        } else {
            printf("get content: %s\n", buf);
            // 模拟数据处理过程
            sleep(5);
        }
    }
    printf("end thread recv data on fd: %d\n", sockfd);
    return 0;
}


int main(int argc, char* argv[])
{
    if(argc <= 2) {
        printf("arg error");
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int ret = 0;

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_port = htons(port);
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);
    ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);
    ret = listen(listenfd, 5);
    assert(ret != -1);

    epoll_event events[MAX_EVENT_NUM];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd, listenfd, false);

    while(1) {
        int ret = epoll_wait(epollfd, events, MAX_EVENT_NUM, -1);

        if(ret < 0) {
            printf("epoll error");
            break;
        }
        
        for(int i = 0; i < ret; ++i) {
            int sockfd = events[i].data.fd;
            if( sockfd == listenfd ) {
                struct sockaddr_in client_address;
                socklen_t client_len = sizeof(client_address);
                int connfd = accept(sockfd, (struct sockaddr*)&client_address, &client_len);
                // 对每个非监听文件描述符都注册epolloneshot事件
                addfd(epollfd, connfd, true);
        
            } else if(events[i].events & EPOLLIN) {
                pthread_t thread;
                fds fds_for_new_worker;
                fds_for_new_worker.epollfd = epollfd;
                fds_for_new_worker.sockfd = sockfd;
                pthread_create(&thread, NULL, work, (void*)&fds_for_new_worker);

            } else {
                printf("something else happened \n");
            }
        }

    }
    close(listenfd);
    return 0;
}
