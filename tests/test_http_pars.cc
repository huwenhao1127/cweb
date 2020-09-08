#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#define BUFFER_SIZE 4096 // 读缓冲区大小

// 主状态机的两种可能状态:
enum CHECK_STATE {
    CHECK_STATE_REQUESTLINE = 0,        // 当前正在分析请求行
    CHECK_STATE_HEADER                  // 当前正在分析头部字段
};

// 从状态机的三种可能状态, 即行的读取状态: 
enum LINE_STATUS {
    LINE_OK = 0,                        // 读取到一个完整的行
    LINE_BAD,                           // 行出错
    LINE_OPEN                           // 行数据尚且不完整
};

// 服务器处理HTTP请求的结果:  
enum HTTP_CODE {
    NO_REQUEST = 0,                      // 请求不完整,需要继续读取客户数据
    GET_REQUEST,                         // 获得了一个完整的客户请求     
    BAD_REQUEST,                         // 客户请求有语法错误
    FORBIDDEN_REQUEST,                   // 客户对资源没有足够的访问权限
    INTERNAL_ERROR,                      // 服务器内部错误
    CLOSED_CONNECTION                    // 客户端已经关闭链接
};

// 简化http的应答报文
static const char* szret[] = {"I get a correct result\n", "Something wrong\n"};

// 从状态机
LINE_STATUS parse_line(char* buffer, int& checked_index, int& read_index)
{
    char temp;

    for(; checked_index < read_index; ++checked_index) {
        // 获取当前要分析的字节
        temp = buffer[checked_index];
        // 如果当前的字节是“\r”, 则说明成功读取到一个完整的行
        if(temp == '\r') {
            // 如果‘\r’字符碰巧是目前buffer中的最后一个已经被读入的客户数据,那么这次分析没有读取到一个完整的行,返回LINE_OPEN
            if((checked_index + 1) == read_index) {
                return LINE_OPEN;
            // 如果下一个字符是'\n',则说明成功读取到一个完整的行
            } else if(buffer[checked_index + 1] == '\n') {
                buffer[checked_index++] = '\0';
                buffer[checked_index++] = '\0';
                return LINE_OK;
            }
            // 否则http请求有语法问题
            return LINE_BAD;
        } else if(temp == '\n') {       // 如果当前的字节是“\n”, 则说明可能读取到一个完整的行
            if( (checked_index > 1) && buffer[checked_index-1] == '\r') {
                buffer[checked_index++] = '\0';
                buffer[checked_index++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

// 分析请求行
HTTP_CODE parse_requestline(char* temp, CHECK_STATE& checkstate)
{   
    // 如果请求行中没有空白字符或“\t”,则http请求必有问题
    char* url = strpbrk(temp, " \t");
    if( !url ) {
        return BAD_REQUEST;
    }

    *url++ = '\0';

    char* method = temp;
    // 仅支持get方法
    if( strcasecmp(method, "GET") == 0 ) {
        printf("the request mthod is: GET\n");
    } else {
        return BAD_REQUEST;
    }

    url += strspn(url, " \t");
    char* version  = strpbrk(url, " \t");
    if( !version ) {
        return BAD_REQUEST;
    }
    *version++ = '\0';
    version += strspn(version, " \t");
    // 仅支持http/1.1
    if( strcasecmp(version, "http/1.1") != 0  ) {
        return BAD_REQUEST;
    }

    // 检查url
    if( strncasecmp(url, "http://", 7) == 0 ) {
        url += 7;
        url = strchr(url, '/');
    }

    if(!url || url[0] != '/' ) {
        return BAD_REQUEST;
    }
    printf("the request url is : %s", url);

    // 状态转移到头部字段的分析
    checkstate = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

// 分析头部字段
HTTP_CODE parse_headers( char* temp ) {
    // 遇到一个空行,说明得到了一个正确的http请求
    if( temp[0] == '\0' ) {
        return GET_REQUEST;
    // 处理host头部字段
    } else if( strncasecmp(temp, "Host:", 5) == 0 ) {
        temp += 5;
        temp += strspn(temp, " \t");
        printf("the host is : %s\n", temp);
    // 其他字段都不处理
    } else {
        printf("can not handle this header\n");
    }
    return NO_REQUEST;
}

// 分析http请求的入口函数
HTTP_CODE parse_content( char* buffer, int& checked_index, CHECK_STATE& checkstate,
                        int& read_index, int& start_line)
{
    LINE_STATUS linestatus = LINE_OK;       // 记录当前行的读取状态
    HTTP_CODE retcode = NO_REQUEST;         // 记录http请求处理结果
    // 主状态机, 用于从buffer中取出所有完整行
    while((linestatus = parse_line(buffer, checked_index, read_index)) == LINE_OK) {
        char* temp = buffer + start_line;
        start_line = checked_index;
        switch (checkstate) {
            case CHECK_STATE_REQUESTLINE: {
                retcode = parse_requestline(temp, checkstate);
                if( retcode == BAD_REQUEST) {
                    return BAD_REQUEST;
                }
                break;
            }
            case CHECK_STATE_HEADER: {
                retcode = parse_headers(temp);
                if(retcode == BAD_REQUEST) {
                    return BAD_REQUEST;
                } else if(retcode == GET_REQUEST) {
                    return GET_REQUEST;
                }
                break;
            }
            default: {
                return INTERNAL_ERROR;
            }
        }
    }
    // 如果没读到一个完整的行, 需要继续读取客户数据
    if(linestatus == LINE_OPEN) {
        return NO_REQUEST;
    } else {
        return BAD_REQUEST;
    }
}

int main(int argc, char* argv[]) 
{
    if(argc <= 2) {
        printf("arg error");
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);
    int ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);
    ret = listen(listenfd, 5);
    assert(ret != -1);

    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);
    int fd = accept(listenfd, (struct sockaddr*)&client, &client_len);

    if(fd < 0) {
        printf("error client fd");
    } else {
        char buffer[BUFFER_SIZE]; 
        memset(buffer, '\0', BUFFER_SIZE);
        int data_read = 0;               
        int read_index = 0;             // 当前已经读了多少字节客户数据
        int checked_index = 0;          // 当前已经分析完多少字节客户数据
        int start_line = 0;             // 行在buffer中的起始位置
        // 设置主状态机的初始状态
        CHECK_STATE checkstate = CHECK_STATE_REQUESTLINE;
        while(1) {
            data_read = recv(fd, buffer + read_index, BUFFER_SIZE - read_index, 0);
            if(data_read == -1) {
                printf("reading failed\n");
                break;
            } else if(data_read == 0) {
                printf("remote client has closed the connection\n");
                break;
            }   
            read_index += data_read;
            HTTP_CODE result = parse_content(buffer, checked_index, checkstate, read_index, start_line);

            if(result == NO_REQUEST) {
                continue;
            } else if(result == GET_REQUEST) {
                send(fd, szret[0], strlen(szret[0]), 0);
                break;
            } else {
                send(fd, szret[1], strlen(szret[1]), 0);
                break;
            }
        }
        close(fd);
    }
    close(listenfd);
    return 0;
}