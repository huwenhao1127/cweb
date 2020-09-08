#ifndef __CWEB_SCHEDULER_H__
#define __CWEB_SCHEDULER_H__

#include "util.h"
#include "mutex.h"
#include "fiber.h"
#include "log.h"
#include "thread.h"

namespace  cweb {

class Scheduler {
public:
	typedef std::shared_ptr<Scheduler> ptr;
	typedef Mutex MutexType;

	Scheduler(size_t threads = 1, bool use_caller = true, const std::string name = "");
	virtual ~Scheduler();

	const std::string& getName() const {return m_name;}
	void start();
	void stop();

	static Scheduler* GetThis();
	static Fiber* GetMainFiber();

	template <class FiberOrCb>
	void schedule(FiberOrCb fc, int thread = -1) {
		bool need_tickle = false;
		{
			MutexType::Lock lock(m_mutex);
			need_tickle  = scheduleNoLock(fc, thread);
		}
		if(need_tickle) {
			tickle();
		}
	}

	template <class InputIterator>
	void schedule(InputIterator begin, InputIterator end) {
		bool need_tickle = false;
		{
			MutexType::Lock lock(m_mutex);
			while(begin != end) {
				need_tickle  = scheduleNoLock(&*begin, -1) || need_tickle;
			}	
		}
		if(need_tickle) {
			tickle();
		}
	}
protected:
	virtual void tickle();
	void run();
	virtual bool stopping();
	virtual void idle();		

	void setThis();
	bool hasIdleThreads() { return m_idleThreadCount > 0;}
private:
	template <class FiberOrCb>
	bool scheduleNoLock(FiberOrCb fc, int thread) {
		bool need_tickle = m_fibers.empty();
		FiberAndThread ft(fc, thread);
		// 如果是有效的任务,就加入队列
		if(ft.fiber || ft.cb) {
			m_fibers.push_back(ft);
		}
		return need_tickle;
	}
private:
	// 待处理的任务结构,要么是fiber,要么时cb,并且要绑定thread
	struct FiberAndThread {
		Fiber::ptr fiber;
		std::function<void()> cb;
		int thread;
		// 将协程放到线程号为thr上的线程上运行
		FiberAndThread(Fiber::ptr f, int thr)
			: fiber(f), thread(thr) {
		}
		// 传指针,避免引用时智能指针引用数加1,防止无法析构协程
		FiberAndThread(Fiber::ptr* f, int thr)
			: thread(thr) {
			fiber.swap(*f);
		}
		FiberAndThread(std::function<void()> func, int thr)
			: cb(func), thread(thr) {
			cb.swap(func);
		}
		FiberAndThread(std::function<void()>* func, int thr)
			: thread(thr) {
			cb.swap(*func);
		}		
		FiberAndThread()
			: thread(-1) {
		}
		void reset() {
			fiber = nullptr;
			thread = -1;
			cb = nullptr;
		}
	};
private:
	std::string m_name;
	std::vector<Thread::ptr> m_threads;
	std::list<FiberAndThread> m_fibers;			//	计划执行的协程
	MutexType m_mutex;
	Fiber::ptr m_rootFiber;
protected:
	std::vector<int> m_threadIds;
	size_t m_threadCount = 0;
	std::atomic<size_t> m_activeThreadCount = {0};
	std::atomic<size_t> m_idleThreadCount = {0};
	bool m_stopping = true;
	bool m_autoStop = false;
	int m_rootThread = 0;

};




}


#endif