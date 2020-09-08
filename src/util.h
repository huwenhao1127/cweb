
#ifndef __CWEB_UTIL_H__
#define __CWEB_UTIL_H__

#include <pthread.h>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <iostream>
#include <unistd.h>
#include <sys/syscall.h>
#include <stdint.h>
#include <list>
#include <sstream>
#include <fstream>
#include <vector>
#include <stdarg.h>
#include <map>
#include <time.h>
#include <semaphore.h>
#include <atomic>
#include <execinfo.h>       
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <string.h>
#include <signal.h>
#include <ucontext.h>


namespace cweb {

pid_t GetThreadId();

uint32_t GetFiberId();

void Backtrace(std::vector<std::string>& bt, int size, int skip = 2);
std::string BacktraceToString(int size, int skip = 2, const std::string& prefix = "    ");


class Noncopyable {
public:
    Noncopyable() = default;
    ~Noncopyable() = default;

    /* 拷贝构造函数(禁用) */
    Noncopyable(const Noncopyable&) = delete;

    /* 赋值函数(禁用) */
    Noncopyable& operator=(const Noncopyable&) = delete;
};

}

#endif
