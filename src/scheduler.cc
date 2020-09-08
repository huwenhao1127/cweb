#include "scheduler.h"
#include "macro.h"

namespace cweb {

	static cweb::Logger::ptr g_logger = CWEB_LOG_NAME("system");

	// 方便线程获取协程调度器的指针
	static thread_local Scheduler* t_scheduler = nullptr;
	static thread_local Fiber* t_scheduler_fiber = nullptr;

	Scheduler::Scheduler(size_t threads, bool use_caller, const std::string name)
		: m_name(name) {
		CWEB_ASSERT(threads > 0);
  
	    if(use_caller) {
	        cweb::Fiber::GetThis();
	        --threads;

	        CWEB_ASSERT(GetThis() == nullptr);
	        t_scheduler = this;

	        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
	        cweb::Thread::SetName(m_name);

	        t_scheduler_fiber = m_rootFiber.get();
	        m_rootThread = cweb::GetThreadId();
	        m_threadIds.push_back(m_rootThread);

	    } else {
			// 当前主线程不放fiber
	        m_rootThread = -1;
	    }
	    m_threadCount = threads;
	}

	Scheduler::~Scheduler() {
	    CWEB_ASSERT(m_stopping);
	    if(GetThis() == this) {
	        t_scheduler = nullptr;
	    }
	}

	Scheduler* Scheduler::GetThis() {
		return t_scheduler;
	}

	Fiber* Scheduler::GetMainFiber() {
		return t_scheduler_fiber;
	}

	/* 启动调度器,初始化线程池 */	
	void Scheduler::start() {
		
		MutexType::Lock lock(m_mutex);
		if(!m_stopping) {
			return;
		}
		m_stopping = false;

		CWEB_ASSERT(m_threads.empty());
		m_threads.resize(m_threadCount);
		for(size_t i = 0; i < m_threadCount; ++i) {
			// 类的成员函数有一个隐含的参数this, 并且不能直接赋值,需要用bind绑定
			m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this),
										m_name + "_" + std::to_string(i)));
			m_threadIds.push_back(m_threads[i]->getId());
		}
		lock.unlock();
	}

	void Scheduler::stop() {
		m_autoStop = true;
		if(m_rootFiber
				&& m_threadCount == 0
				&& (m_rootFiber->getState() == Fiber::INIT 
					|| m_rootFiber->getState() == Fiber::TERM)) {
			CWEB_LOG_INFO(g_logger) << this << ":  stopped";
			m_stopping = true;

			if(stopping()) {
				return;
			}
		}

		//bool exit_on_this_fiber = false;
		if(m_rootThread != -1) {
			CWEB_ASSERT(GetThis() == this);
		} else {
			CWEB_ASSERT(GetThis() != this);
		}

		m_stopping = true;
		for(size_t i = 0; i < m_threadCount; ++i) {
			tickle(); // 唤醒线程并结束
		}
		// 结束主线程
		if(m_rootFiber) {
			tickle();
		}

		if(m_rootFiber) {
			if(!stopping()) {
				m_rootFiber->call();
			}
		}

		std::vector<Thread::ptr> thrs;
		{
			MutexType::Lock lock(m_mutex);
			thrs.swap(m_threads);
		}

		for(auto& i : thrs) {
			i->join();
		}
	}

	void Scheduler::setThis() {
		t_scheduler = this;
	}

	void Scheduler::run() {
    	CWEB_LOG_DEBUG(g_logger) << m_name << " run";
		setThis();

		/* 设置scheduler fiber 为 线程上的main fiber */
		if(cweb::GetThreadId() != m_rootThread) {
	        t_scheduler_fiber = Fiber::GetThis().get();
		}

	    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
	    Fiber::ptr cb_fiber;
		
	    FiberAndThread ft;
	    while(true) {
	        ft.reset();
	        bool tickle_me = false;
	        bool is_active = false;
	        {
	            MutexType::Lock lock(m_mutex);
	            auto it = m_fibers.begin();
	            while(it != m_fibers.end()) {
	                if(it->thread != -1 && it->thread != cweb::GetThreadId()) {
	                    ++it;
	                    tickle_me = true;
	                    continue;
	                }

	                CWEB_ASSERT(it->fiber || it->cb);
	                if(it->fiber && it->fiber->getState() == Fiber::EXEC) {
	                    ++it;
	                    continue;
	                }

	                ft = *it;
	                m_fibers.erase(it++);
	                ++m_activeThreadCount;
	                is_active = true;
	                break;
	            }
	            tickle_me |= it != m_fibers.end();
	        }

	        if(tickle_me) {
	            tickle();
	        }

	        if(ft.fiber && (ft.fiber->getState() != Fiber::TERM
	                        && ft.fiber->getState() != Fiber::EXCEPT)) {
	            ft.fiber->swapIn();
	            --m_activeThreadCount;

	            if(ft.fiber->getState() == Fiber::READY) {
	                schedule(ft.fiber);
	            } else if(ft.fiber->getState() != Fiber::TERM
	                    && ft.fiber->getState() != Fiber::EXCEPT) {
	                ft.fiber->setState(Fiber::HOLD);
	            }
	            ft.reset();
	        } else if(ft.cb) {
	            if(cb_fiber) {
	                cb_fiber->reset(ft.cb);
	            } else {
	                cb_fiber.reset(new Fiber(ft.cb));
	            }
	            ft.reset();
	            cb_fiber->swapIn();
	            --m_activeThreadCount;
	            if(cb_fiber->getState() == Fiber::READY) {
	                schedule(cb_fiber);
	                cb_fiber.reset();
	            } else if(cb_fiber->getState() == Fiber::EXCEPT
	                    || cb_fiber->getState() == Fiber::TERM) {
	                cb_fiber->reset(nullptr);
	            } else {
	                cb_fiber->setState(Fiber::HOLD);
	                cb_fiber.reset();
	            }
	        } else {
	            if(is_active) {
	                --m_activeThreadCount;
	                continue;
	            }
	            if(idle_fiber->getState() == Fiber::TERM) {
	                CWEB_LOG_INFO(g_logger) << "idle fiber term";
	                break;
	            }

	            ++m_idleThreadCount;
	            idle_fiber->swapIn();
	            --m_idleThreadCount;
	            if(idle_fiber->getState() != Fiber::TERM
	                    && idle_fiber->getState() != Fiber::EXCEPT) {
	                idle_fiber->setState(Fiber::HOLD);
	            }
	        }
	    }
	}

	void Scheduler::tickle() {
    	CWEB_LOG_INFO(g_logger) << "tickle";
	}

	bool Scheduler::stopping() {
	    MutexType::Lock lock(m_mutex);
	    return m_autoStop && m_stopping
	        && m_fibers.empty() && m_activeThreadCount == 0;
	}

	void Scheduler::idle() {
	    CWEB_LOG_INFO(g_logger) << "idle";
	    while(!stopping()) {
	        cweb::Fiber::YieldToHold();
	    }
	}


}