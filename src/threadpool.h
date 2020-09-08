#ifndef __CWEB_PROCESSPOOL_H__
#define __CWEB_PROCESSPOOL_H__

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "mutex.h"


/* 半同步/半反应堆线程池 */
namespace cweb {

// T 是任务类
template<typename T>
class ThreadPool {
public:
    typedef cweb::Mutex Mutex;
    // 输入参数是线程数和请求队列的最大长度
    ThreadPool(int thread_number = 16, int max_requests = 50);
    ~ThreadPool();
    bool append(T* request);
private:
    // 工作线程运行的函数
    static void* worker(void* arg);
    void run();
private:
    int m_thread_number;            // 线程数
    int m_max_requests;             // 请求队列最大长度
    bool m_stop;                    // 是否结束线程
    pthread_t* m_threads;           // 线程池
    std::list<T*> m_workqueue;      // 请求队列
    Mutex m_queuelocker;            // 保护请求队列的互斥锁
    Semaphore m_queuestat;          // 是否有任务需要处理
};

template<typename T>
ThreadPool<T>::ThreadPool(int thread_number, int max_requests)
    : m_thread_number(thread_number), m_max_requests(max_requests),
        m_stop(false), m_threads(NULL)
{
    if( (thread_number <= 0) || (max_requests <= 0) ) {
        throw std::exception();
    }

    m_threads = new pthread_t[thread_number];
    if(!m_threads) {
        throw std::exception();
    }

    // 创建线程池,并将它们设置为脱离线程
    for(int i = 0; i < thread_number; ++i) {
        printf("create the %dth threads\n", i);
        
        if(pthread_create(m_threads + i, NULL, worker, this)) {
            delete [] m_threads;
            throw std::exception();
        }

        // 分离子线程
        if( pthread_detach(m_threads[i])) {
            delete [] m_threads;
            throw std::exception();
        }
    }
}

template<typename T>
ThreadPool<T>::~ThreadPool() {
    delete [] m_threads;
    m_stop = true;
}

template<typename T>
bool ThreadPool<T>::append(T* request) {
    // 操作工作队列时加锁
    {
        Mutex::Lock lck(m_queuelocker);
        if(m_workqueue.size() > m_max_requests) {
            lck.unlock();
            return false;
        }
        m_workqueue.push_back(request);
    }
    m_queuestat.notify();
    return true;
}

template<typename T>
void* ThreadPool<T>::worker(void* arg) {
    ThreadPool* pool = (ThreadPool*) arg;
    pool->run();
    return pool;
}

template<typename T>
void ThreadPool<T>::run() {
    while (!m_stop) {
        m_queuestat.wait();
        T* request;
        {
            Mutex::Lock lck(m_queuelocker);
            if(m_workqueue.empty()) {
                lck.unlock();
                continue;
            }
            request = m_workqueue.front();
            m_workqueue.pop_front();
        }
        if(! request) {
            continue;
        }
        request->process();
    }
}


}
#endif