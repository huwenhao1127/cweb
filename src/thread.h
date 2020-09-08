
#ifndef __CWEB_THREAD_H__
#define __CWEB_THREAD__H__

#include "util.h"
#include "mutex.h"

namespace cweb {

class Thread : Noncopyable{
public:
    typedef std::shared_ptr<Thread> ptr;
    Thread(std::function<void()> cb, const std::string& name);
    ~Thread();

    pid_t getId() const {return m_id;}
    const std::string& getName() const {return m_name;}

    void join();

    static Thread* GetThis();
    static const std::string& GetName();
    static void SetName(const std::string& name);
    // 线程入口函数

private:
    static void* run(void* arg);

private:
    pid_t m_id = -1;
    pthread_t m_thread = 0;
    std::function<void()> m_cb;
    std::string m_name;

    // 在构造函数中,可能类已经构造完成了,但pthread_create()还没有执行完成,
    // 所以构造完成时要wait,等初始化之后再notify
    Semaphore m_semaphore;
};


}

#endif
