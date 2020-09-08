#include "http_conn.h"

/* http响应的状态信息 */
const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form = "You Request has bad syntax or is inherently i,possible to satisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form = "You dont have permission to get file from this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form = "The requested file not found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual problem serving the requested file.\n";

const char* doc_root = "/var/www/html";

namespace cweb {

int HTTPCONN::m_epollfd = -1;
int HTTPCONN::m_user_count = 0;

int setnonblocking(int fd) {
    int old_opt = fcntl(fd, F_GETFL);
    int new_opt = old_opt | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_opt);\
    return old_opt;
}

void addfd(int epollfd, int fd, bool one_shot) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    if(one_shot) {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

void modfd(int epollfd, int fd, int ev) {
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

void HTTPCONN::SetEpoll(int epollfd) {
    HTTPCONN::m_epollfd = epollfd;
}


HTTPCONN::HTTPCONN(){}

HTTPCONN::~HTTPCONN(){}

void HTTPCONN::init(int sockfd, const sockaddr_in& addr) {
    m_sockfd = sockfd;
    m_address = addr;

    /* reuse方便调试的时候用 */
    //int reuse = 1;
    //setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // 注册读事件
    addfd(m_epollfd, m_sockfd, true);
    m_user_count++;

    init();
}

void HTTPCONN::init() {
    m_checked_state = CHECK_STATE_REQUESTLINE;
    m_linger = false;

    m_method = GET;
    m_url = 0;
    m_version = 0;
    m_content_length = 0;
    m_host = 0;
    m_start_line = 0;
    m_checked_idx = 0;
    m_read_idx = 0;
    m_write_idx = 0;

    memset(m_read_buf, '\0', READ_BUFFER_SIZE);
    memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
    memset(m_real_file, '\0', FILENAME_LEN);
}


void HTTPCONN::close_conn(bool real_close) {
    if(real_close && m_sockfd != -1) {
        // printf("close %d\n", m_sockfd);
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        m_user_count--;
    }
}

// 由线程池中工作线程调用,处理http请求的入口函数
void HTTPCONN::process() {
    HTTPCONN::HTTP_CODE read_ret = process_read();
    if(read_ret == NO_REQUEST) {
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        return;
    }

    bool write_ret = process_write(read_ret);
    if(!write_ret) {
        close_conn(true);
    }
    // 注册写完成事件
    modfd(m_epollfd, m_sockfd, EPOLLOUT);
}

// 根据服务器处理HTTP请求的结果,决定返回给客户端的内容
bool HTTPCONN::process_write(HTTPCONN::HTTP_CODE ret) {
    switch(ret) {
        case INTERNAL_ERROR: {
            add_status_line(500, error_500_title);
            add_headers(strlen(error_500_form));
            if(!add_content(error_500_form)) {
                return false;
            }
            break;
        }
        case BAD_REQUEST: {
            add_status_line(400, error_400_title);
            add_headers(strlen(error_400_form));
            if(!add_content(error_400_form)) {
                return false;
            }
            break;
        }
        case NO_RESOURCE: {
            add_status_line(404, error_404_title);
            add_headers(strlen(error_404_form));
            if(!add_content(error_404_form)) {
                return false;
            }
            break;
        }
        case FORBIDDEN_REQUEST: {
            add_status_line(403, error_403_title);
            add_headers(strlen(error_403_form));
            if(!add_content(error_403_form)) {
                return false;
            }
            break;
        }
        case FILE_REQUEST: {
            add_status_line(200, ok_200_title);
            if( m_file_stat.st_size != 0) {
                add_headers(m_file_stat.st_size);
                m_iv[0].iov_base = m_write_buf;
                m_iv[0].iov_len = m_write_idx;
                m_iv[1].iov_base = m_file_address;
                m_iv[1].iov_len = m_file_stat.st_size;
                m_iv_count = 2;   
                return true;          
            } else {
                const char* ok_string = "<html><body></body></html>";
                add_headers(strlen(ok_string));
                if(!add_content(ok_string)) {
                    return false;
                }
            }
        }
        default: {
            return false;
        }
    }
    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_idx;
    m_iv_count = 1;
    return true;
}

// 将http报文读入缓冲区 et模式
bool HTTPCONN::read() {
    if(m_read_idx >= READ_BUFFER_SIZE) {
        return false;
    } 
    int bytes_read = 0;
    while(true) {
        bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        if( bytes_read == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            return false;
        } else if(bytes_read == 0) {
            return false;
        }
        // 没读完
        m_read_idx += bytes_read;
    }
    return true;
}


// 分析http请求行,获得请求方法、目标URL、以及HTTP版本号
HTTPCONN::HTTP_CODE HTTPCONN::parse_request_line(char* text) {
    m_url = strpbrk(text, " \t");
    if(!m_url) {
        return BAD_REQUEST;
    }
    *m_url++ = '\0';

    char* method = text;
    if(strcasecmp(method, "GET") == 0) {
        m_method = GET;
    } else {
        return BAD_REQUEST;
    }

    m_url += strspn(m_url, " \t");
    m_version = strpbrk(m_url, " \t");
    if(!m_version) {
        return BAD_REQUEST;
    }
    *m_version++ = '\0';    
    m_version += strspn(m_version, " \t");
    if( strcasecmp(m_version, "HTTP/1.1") != 0 ) {
        if( strcasecmp(m_version, "HTTP/1.0") != 0 ) {
            return BAD_REQUEST;
        } 
    }
    
    if( strncasecmp(m_version, "http://", 7) == 0) {
        m_url += 7;
        m_url = strchr(m_url, '/');
    }

    if( strncasecmp(m_version, "https://", 8) == 0) {
        m_url += 8;
        m_url = strchr(m_url, '/');
    }

    if(!m_url || m_url[0] != '/') {
        return BAD_REQUEST;
    }

    if (strlen(m_url) == 1)
        strcat(m_url, "test.html");
    
    m_checked_state= CHECK_STATE_HEADER;
    return NO_REQUEST;
}

// 解析头部信息
HTTPCONN::HTTP_CODE HTTPCONN::parse_headers(char* text) {
    // 遇到空行,表示头部字段解析完毕
    if(text[0] == '\0') {
        // 如果http请求有消息体,则还需要读取m_content_length字节的消息体,状态机转移到CHECK_STATE_CONTENT状态
        if( m_content_length != 0 ) {
            m_checked_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        // 否则我们得到了完整的http请求
        return GET_REQUEST;

    // connection头部    
    } else if( strncasecmp(text, "Connection:", 11) == 0) {
        text += 11;
        text += strspn(text, " \t");
        if( strcasecmp(text, "keep-alive") == 0) {
            m_linger = true;
        }
    // 处理content-length头部    
    } else if( strncasecmp(text, "Content-Length:", 15) == 0) {
        text += 15;
        text += strspn(text, " \t");
        m_content_length = atol(text);
    // 处理host头部
    } else if( strncasecmp(text, "Host:", 5) == 0)  {
        text += 5;
        text += strspn(text, " \t");
        m_host = text;
    } else {
        //printf("oop! unknow header: %s\n", text);
    }
    return NO_REQUEST;
}

// 不真正解析请求的消息体
HTTPCONN::HTTP_CODE HTTPCONN::parse_content(char* text) {
    if(m_read_idx >= (m_content_length + m_checked_idx)) {
        text[m_content_length] ='\0';
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

// 得到一个完整、正确的HTTP请求时,分析目标文件的属性,若目标文件没有问题,用mmap将其映射到m_file_address处
HTTPCONN::HTTP_CODE HTTPCONN::do_request() {
    //printf("do request\n");
    //printf("url: %s\n", m_url);
    strcpy(m_real_file, doc_root);
    int len = strlen(doc_root);
    strncpy(m_real_file + len, m_url, FILENAME_LEN - len -1);
    // printf("file name: %s\n", m_real_file);
    if( stat(m_real_file, &m_file_stat) < 0) {
        return NO_REQUEST;
    }
    if( !(m_file_stat.st_mode & S_IROTH) ) {
        return FORBIDDEN_REQUEST;
    }
    if( (S_ISDIR(m_file_stat.st_mode)) ) {
        return BAD_REQUEST;
    }
    int fd = open(m_real_file, O_RDONLY);
    m_file_address = (char*)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    return FILE_REQUEST;
}   

HTTPCONN::LINE_STATUS HTTPCONN::parse_line() {
    char temp;
    for(; m_checked_idx < m_read_idx; ++m_checked_idx) {
        temp = m_read_buf[m_checked_idx];
        // 如果当前的字节是“\r”, 则说明成功读取到一个完整的行
        if(temp == '\r') {
            // 如果‘\r’字符碰巧是目前buffer中的最后一个已经被读入的客户数据,那么这次分析没有读取到一个完整的行,返回LINE_OPEN
            if((m_checked_idx + 1) == m_read_idx) {
                return LINE_OPEN;
            // 如果下一个字符是'\n',则说明成功读取到一个完整的行
            } else if(m_read_buf[m_checked_idx + 1] == '\n') {
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            // 否则http请求有语法问题
            return LINE_BAD;
        // 如果当前的字节是“\n”, 则说明可能读取到一个完整的行
        } else if(temp == '\n') {       
            if( (m_checked_idx > 1) && m_read_buf[m_checked_idx-1] == '\r') {
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

// 分析http请求的入口函数 主状态机
HTTPCONN::HTTP_CODE HTTPCONN::process_read() {
    HTTPCONN::LINE_STATUS line_status = LINE_OK;       // 记录当前行的读取状态
    HTTPCONN::HTTP_CODE ret = NO_REQUEST;         // 记录http请求处理结果
    char* text = 0;

    while( ((m_checked_state == CHECK_STATE_CONTENT) && (line_status == LINE_OK))
        || ((line_status = parse_line() ) == LINE_OK)) {
        text = get_line();
        m_start_line = m_checked_idx;
        //printf("got 1 http line: %s\n", text);
        // printf("check state: %d\n\n", m_checked_state);
        switch (m_checked_state) {
            case CHECK_STATE_REQUESTLINE: {
                ret = parse_request_line(text);
                if( ret == BAD_REQUEST) {
                    return BAD_REQUEST;
                }
                break;
            }   
            case CHECK_STATE_HEADER: {
                ret = parse_headers(text);
                if( ret == BAD_REQUEST) {
                    return BAD_REQUEST;
                } else if( ret == GET_REQUEST) {
                    return do_request();
                }
                break;
            }
            case CHECK_STATE_CONTENT: {
                ret = parse_content(text);
                if( ret == GET_REQUEST) {
                    return do_request();
                } 
                line_status = LINE_OPEN;
                break;
            }
            default: {
                return INTERNAL_ERROR;
            }
        }
        
    }
    return NO_REQUEST;
}

// 对内存区执行munmap
void HTTPCONN::unmap() {
    if(m_file_address) {
        munmap(m_file_address, m_file_stat.st_size);
        m_file_address = 0;
    }
}

// 发送write_buf里的内容
bool HTTPCONN::write() {
    int temp = 0;
    int bytes_have_send = 0;
    int bytes_to_send = m_write_idx;
    if( bytes_to_send == 0 ) {
        printf("bytes to send = 0\n");
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        init();
        return true;
    }
    // printf("bytes to send: %d\n",  bytes_to_send);
    // printf("bytes have send: %d\n", bytes_have_send);
    // printf("header: %s", (char*)m_iv[0].iov_base);
    // printf("content: %s\n", (char*)m_iv[1].iov_base);
    // printf("count: %d\n", m_iv_count);
    while(1) {
        temp = writev(m_sockfd, m_iv, m_iv_count);


        if(temp <= -1) {
            // TCP写缓存没有空间,等待下一轮EPOLLOUT事件
            if(errno == EAGAIN) {
                modfd(m_epollfd, m_sockfd, EPOLLOUT);
                return true;
            }
            unmap();
            return false;
        }
       
        bytes_to_send -= temp;
        bytes_have_send += temp;

        if (bytes_have_send >= (int)m_iv[0].iov_len)
        {
            m_iv[0].iov_len = 0;
            m_iv[1].iov_base = m_file_address + (bytes_have_send - m_write_idx);
            m_iv[1].iov_len = bytes_to_send;
        }
        else
        {
            m_iv[0].iov_base = m_write_buf + bytes_have_send;
            m_iv[0].iov_len = m_iv[0].iov_len - bytes_have_send;
        }

        if(bytes_to_send <= 0) {
            // printf("bytes to send: %d\n",  bytes_to_send);
            // printf("bytes have send: %d\n", bytes_have_send);
            // 发送http响应成功,根据http请求中对connection字段判断是否立即关闭链接
            unmap();
            if(m_linger) {
                init();
                //modfd(m_epollfd, m_sockfd, EPOLLIN);
                return true;
            } else {
                //modfd(m_epollfd, m_sockfd, EPOLLIN);
                return false;
            }
        }
    }   
}

// 往写缓存中写入待发送的数据
bool HTTPCONN::add_response(const char* format, ...) {
    if(m_write_idx >= WRITE_BUFFER_SIZE) {
        return false;
    }
    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE-1-m_write_idx, format, arg_list);
    
    if( len >= (WRITE_BUFFER_SIZE - 1 - m_write_idx)) {
        return false;
    }

    m_write_idx += len;
    va_end(arg_list);
    return true;
}

bool HTTPCONN::add_content(const char* content) {
    return add_response("%s", content);
}

bool HTTPCONN::add_status_line(int status, const char* title) {
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool HTTPCONN::add_headers(int content_length) {
    return (add_content_length(content_length)
        && add_linger() && add_blank_line());
}

bool HTTPCONN::add_content_length(int content_length) {
    return add_response("Content-Length: %d\r\n", content_length);
}

bool HTTPCONN::add_linger() {
    return add_response("Connection: %s\r\n", (m_linger == true) ? "keep-alive" : "close");
}

bool HTTPCONN::add_blank_line() {
    return add_response("%s", "\r\n");
}



}
