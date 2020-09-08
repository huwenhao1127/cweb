#ifndef __CWEB_PROCESSPOOL_H__
#define __CWEB_PROCESSPOOL_H__

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
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>

namespace cweb {

// 子进程
class process {
public:
    process() : m_pid(-1) {};

public:
    pid_t m_pid;
    int m_pipefd[2];
}
    
// 进程池类
template <typename T>
class processpool {
private:
    // 构造函数定义为私有,只能通过create静态函数来创建processpool实例
    processpool(int listenfd, int process_number = 8);
public:
    // 单例模式
    static processpool<T>* create(int listenfd, int process_number = 8)
    {
        if(!m_instance) {
            m_instance = new processpool<T>(listenfd, process_number);
        }
        return m_instance;
    }
    ~processpool() {
        delete [] m_sub_process;
    }

    // 启动进程池
    void run();

private:
    void setuo_sig_pipe();
    void run_parent();
    void run_child();

private:
    // 进程池允许的最大子进程数量
    static const int MAX_PROCESS_NUMBER = 16;
    // 每个子进程最多能处理的客户数量
    static const int USER_PER_PROCESS = 65536;
    // epoll最多能处理的事件数
    static const int MAX_EVENT_NUMBER = 10000;
    // 进程池中的进程总数
    int m_process_number;
    // 子进程在池中的序号
    int m_idx;
    // 每个进程都有一个epoll内核事件表
    int m_epollfd;
    // 监听socket
    int m_listenfd;
    // 子进程通过m_stop来决定是否停止运行   
    int m_stop;
    // 保存所有子进程的描述信息
    process* m_sub_process;
    // 进程池静态实例
    static processpool<T>* m_instance;
};

template <typename T>
processpool<T>* processpool<T>::m_instance = NULL;

// 信号管道: 用于处理信号的管道,以实现统一事件源
static int setuo_sig_pipefd[2];

static int setnonblocking(int fd)
{
    int old_opt = fcntl(fd, F_GETFL);
    int new_opt = old_opt | O_NONBLOCKING;
    fcntl(fd, F_SETFL, new_opt);
    return old_opt;
}

static void addfd(int epollfd, int sockfd)
{
    epoll_event event;
    event.data.fd = sockfd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &event);
    setnonblocking(sockfd);
}

static void removefd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

static void sig_handler(int sig)
{
    int save_errno = errno;
    int msg = sig;
    send(sig_pipefd[1], (char*)&msg, 1, 0);
    errno = save_errno;
}

static void addsig(int sig, void(handler)(int), bool restart = true)
{
    return;
}





}



#endif