#ifndef __CWEB_HTTPCONNECTION_H__
#define __CWEB_HTTPCONNECTION_H__

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <memory>
//#include "mutex.h"

namespace cweb {

int setnonblocking(int fd);
void addfd(int epollfd, int fd, bool one_shot);
void removefd(int epollfd, int fd);
void modfd(int epollfd, int fd, int ev);

class HTTPCONN {
public:
    
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;

    enum METHOD {   
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATCH
    };

    enum CHECK_STATE {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };

    enum HTTP_CODE {
        NO_REQUEST = 0,                      // 请求不完整,需要继续读取客户数据
        GET_REQUEST,                         // 获得了一个完整的客户请求     
        BAD_REQUEST,                         // 客户请求有语法错误
        NO_RESOURCE,                         // 没有资源
        FORBIDDEN_REQUEST,                   // 客户对资源没有足够的访问权限
        FILE_REQUEST,                        //
        INTERNAL_ERROR,                      // 服务器内部错误
        CLOSED_CONNECTION                    // 客户端已经关闭链接
    };

    enum LINE_STATUS {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

public:
    typedef std::shared_ptr<HTTPCONN> ptr;
    HTTPCONN();
    ~HTTPCONN();

public:
    void init();
    void init(int sockfd, const sockaddr_in& addr);
    void close_conn(bool real_close = true);
    void process();
    bool read();
    bool write();
    HTTP_CODE process_read();
    bool process_write(HTTP_CODE ret);
    static void SetEpoll(int epollfd);

    // 分析http请求
    HTTP_CODE parse_request_line(char* text);
    HTTP_CODE parse_headers(char* text);
    HTTP_CODE parse_content(char* text);
    HTTP_CODE do_request();
    char* get_line() {return m_read_buf + m_start_line;}
    LINE_STATUS parse_line();

    // 填充http应答
    void unmap();
    bool add_response(const char* format, ...);
    bool add_content(const char* content);
    bool add_status_line(int status, const char* title);
    bool add_headers(int content_length);
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

public:
    // 所有socket上的事件都被注册到同一个饿poll内核事件表,所以将epoll文件描述符设置为静态
    static int m_epollfd;
    static int m_user_count;

private:
    int m_sockfd;
    sockaddr_in m_address;
    char m_read_buf[READ_BUFFER_SIZE];
    int m_read_idx;
    int m_checked_idx;
    int m_start_line;       // 当前正在解析行的起始位置

    char m_write_buf[WRITE_BUFFER_SIZE];
    int m_write_idx;        // 写缓冲区待发送的字节数

    CHECK_STATE m_checked_state;
    METHOD m_method;

    /* 客户请求目标文件的完整路径,其内容等于doc_root + m_url, doc_root是网站根目录 */
    char m_real_file[FILENAME_LEN];
    char* m_url;    // 请求目标文件文件名
    char* m_version;
    char* m_host;
    int m_content_length;    // http请秋消息体的长度
    bool m_linger;          // 是否要求保持链接

    /* 客户请求的目标文件被mmap到内存中的起始位置 */
    char* m_file_address;
    /* 目标文件的状态.通过它判断文件是否存在、是否为目录、是否可读 */
    struct stat m_file_stat;

    /* 用writev执行写操作 */
    struct iovec m_iv[2];
    int m_iv_count;
};







}





#endif