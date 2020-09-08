#include "thread.h"
#include "log.h"

namespace cweb {
    
static thread_local Thread* t_thread = nullptr;
static thread_local std::string t_thread_name = "UNKNOW";
static cweb::Logger::ptr g_logger =  CWEB_LOG_NAME("sysyem");

/* ------------------------------------------thread------------------------------------------------- */

Thread* Thread::GetThis() {
    return t_thread;
}

const std::string& Thread::GetName() {
    return t_thread_name;
}

void Thread::SetName(const std::string& name) {
    if(name.empty()) {
        return;
    }
    if(t_thread) {
        t_thread->m_name = name;
    }
    t_thread_name = name;
}

void* Thread::run(void* arg) {
    // 线程运行cb前,将环境变量设置为自己
    Thread* thread = (Thread*)arg;
    t_thread = thread;
    t_thread_name = thread->m_name;

    // pthread_self()获取的tid由线程库维护,不同进程之间的线程tid可能重复,gettid获取的是全局唯一的线程id
    thread->m_id = syscall(SYS_gettid);
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

    std::function<void()> cb;
    cb.swap(thread->m_cb);

    thread->m_semaphore.notify();

    cb();
    return 0;    
}

Thread::Thread(std::function<void()> cb, const std::string& name)
    : m_cb(cb), m_name(name)
{
    if(name.empty()) {
        m_name = "UNKNOW";
    }
    int ret = pthread_create(&m_thread, nullptr, &Thread::run, this);
    if(ret) {
        CWEB_LOG_ERROR(g_logger) << "pthread_create thread fail, rt=" << ret
            << " name=" << name;
        throw std::logic_error("pthread_create error");
    }
    m_semaphore.wait();
}

Thread::~Thread() {
    if(m_thread) {
        pthread_detach(m_thread);
    }
}

void Thread::join() {
    if(m_thread) {
        int ret = pthread_join(m_thread, nullptr);
        if(ret) {
            CWEB_LOG_ERROR(g_logger) << "pthread_join thread fail, rt=" << ret
                << " name=" << m_name;
            throw std::logic_error("pthread_join error");
        }
        m_thread = 0;
    }
}


}

