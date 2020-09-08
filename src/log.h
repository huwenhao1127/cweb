#ifndef __CWEB_LOG_H__
#define __CWEB_LOG_H__

#include <syslog.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <string>

#include <memory>

#define CWEB_LOG_NAME() \

#define CWEB_LOG_INFO() \


namespace cweb {

class Logger {
public:
    typedef shared_ptr<Logger> ptr;
    Logger(string name, string log_path);
    ~Logger();

    setLevel();
    addLog();

public:
    enum Level{
        EMERG 0		// 系统不可用
        ALERT 1		// 报警
        CRIT 2		// 非常严重的情况
        ERR 3		// 错误
        WARNING 4		// 警告
        NOTICE 5		// 通知
        INFO 6		// 信息
        DEBUG 7		// 调试
    };

private:
    string m_name;
    string m_log_path; 
    Level level = DEBUG;
}

}


#endif