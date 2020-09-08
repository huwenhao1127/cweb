#ifndef __CWEB_FIBER_H__
#define __CWEB_FIBER_H__

#include <functional>
#include <ucontext.h>
#include <memory>

namespace cweb {

class Scheduler;

class Fiber : public std::enable_shared_from_this<Fiber> {
public:
	typedef std::shared_ptr<Fiber> ptr;
	enum State {
		INIT,	// 初始化
		HOLD,	// 暂停
		EXEC,	// 执行
		TERM,	// 结束
		READY,	// 可执行
		EXCEPT	// 异常
	};

private:
	Fiber();

public:
	Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
	~Fiber();

	void reset(std::function<void()> cb);			// 	重制协程函数,并重制状态
	void swapIn();									// 	切换到当前协程执行
	void swapOut();									// 	切换到后台执行
	uint64_t getId() const {return m_id;}
	State getState() const { return m_state;}
	void setState(State sta) {m_state = sta;}

	void call();
	void back();

	static void SetThis(Fiber* f);					// 	设置主协程
	static Fiber::ptr GetThis();					// 	获取主协程,线程初始化之后的第一个协程
	static void YieldToReady();						// 	协程切换到后台,并设置为Ready状态
	static void YieldToHold();						// 	协程切换到后台,并设置为Hold状态
	static uint64_t TotalFibers();					// 	总协程数

	static void MainFunc();
	static void CallerMainFunc();
	static uint64_t GetFiberId();

private:									
	uint64_t m_id = 0;
	uint64_t m_stacksize = 0;
	State m_state = INIT;
	
	ucontext_t m_ctx;
	void* m_stack = nullptr;
	std::function<void()> m_cb;

};

}


#endif
