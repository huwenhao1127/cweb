#include "fiber.h"
#include "macro.h"
#include <atomic>
#include "log.h"
#include "scheduler.h"


namespace cweb {

static Logger::ptr g_logger = CWEB_LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id {0};
static std::atomic<uint64_t> s_fiber_count {0};

static thread_local Fiber* t_fiber = nullptr;			// 当前正在执行的协程
static thread_local Fiber::ptr t_threadFiber = nullptr; // main 协程

static uint64_t g_fiber_stack_size = 1024 * 1024;		// 协程栈大小

/* 内存分配器 */
class MallocStackAllocator {
public:
	static void* Alloc(size_t size) {
		return malloc(size);
	}

	static void Dealloc(void* vp, size_t size) {
		return free(vp);
	}
};

typedef MallocStackAllocator StackAllocator;

uint64_t Fiber::GetFiberId() {
	if(t_fiber) {
		return t_fiber->getId();
	}
	return 0;
}

Fiber::Fiber() {
	m_state = EXEC;
	SetThis(this);  // 将this赋给t_fiber

	// 获取线程上下文
	if(getcontext(&m_ctx)) {
		CWEB_ASSERT2(false, "getcontext");
	}

	++s_fiber_count;
	CWEB_LOG_DEBUG(g_logger) << "Fiber::Fiber main()";
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
	:m_id(++s_fiber_id), m_cb(cb)
{
	++s_fiber_count;
	m_stacksize = stacksize ? stacksize : g_fiber_stack_size;

	m_stack = StackAllocator::Alloc(m_stacksize);
	if(getcontext(&m_ctx)) {
		CWEB_ASSERT2(false, "getcontext");
	}
	m_ctx.uc_link = nullptr;
	m_ctx.uc_stack.ss_sp = m_stack;
	m_ctx.uc_stack.ss_size = m_stacksize;

    if(!use_caller) {
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
    } else {
        makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
    }

    CWEB_LOG_DEBUG(g_logger) << "Fiber::Fiber() id = " << m_id;
}

Fiber::~Fiber() {
	--s_fiber_count;
	if(m_stack) {
		CWEB_ASSERT(m_state == TERM || m_state == INIT || m_state == EXCEPT);
		StackAllocator::Dealloc(m_stack, m_stacksize);
	} else {
        /* 析构main fiber */
		CWEB_ASSERT(!m_cb);
		CWEB_ASSERT(m_state == EXEC);

		Fiber* cur = t_fiber;
		if(cur == this) {
			SetThis(nullptr);
		}
	}
	CWEB_LOG_DEBUG(g_logger) << "Fiber::~Fiber()  id = " << m_id
                              << "  total = " << s_fiber_count;
}

//重置协程函数，并重置状态
//INIT，TERM, EXCEPT
void Fiber::reset(std::function<void()> cb) {
	CWEB_ASSERT(m_stack);
	CWEB_ASSERT(m_state == TERM || m_state == INIT || m_state == EXCEPT);
	m_cb = cb;
	if(getcontext(&m_ctx)) {
		CWEB_ASSERT2(false, "getcontext");
	}

	m_ctx.uc_link = nullptr;
	m_ctx.uc_stack.ss_sp = m_stack;
	m_ctx.uc_stack.ss_size = m_stacksize;

	makecontext(&m_ctx, &Fiber::MainFunc, 0);
	m_state = INIT;
}

void Fiber::call() {
    SetThis(this);
    m_state = EXEC;
    if(swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
        CWEB_ASSERT2(false, "swapcontext");
    }
}

void Fiber::back() {
    SetThis(t_threadFiber.get());
    if(swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
        CWEB_ASSERT2(false, "swapcontext");
    }
}

//切换到当前协程执行
void Fiber::swapIn() {
    SetThis(this);
    CWEB_ASSERT(m_state != EXEC);
    m_state = EXEC;
    if(swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {
        CWEB_ASSERT2(false, "swapcontext");
    }
}

//切换到后台执行
void Fiber::swapOut() {
    SetThis(Scheduler::GetMainFiber());
    if(swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
        CWEB_ASSERT2(false, "swapcontext");
    }
}

void Fiber::SetThis(Fiber* f) {
	t_fiber = f;
}

Fiber::ptr Fiber::GetThis() {
	if(t_fiber) {
		return t_fiber->shared_from_this();
	}
	Fiber::ptr main_fiber(new Fiber);
	CWEB_ASSERT(t_fiber == main_fiber.get());
	t_threadFiber = main_fiber;
	return t_fiber->shared_from_this();
}

void Fiber::YieldToReady() {
	Fiber::ptr cur = GetThis();
	CWEB_ASSERT(cur->m_state == EXEC);
	cur->m_state = READY;
    //cur->swapOut();
    if(Scheduler::GetMainFiber()) {
        cur->swapOut();
    } else {
        cur->back();
    }
}	

void Fiber::YieldToHold() {
	Fiber::ptr cur = GetThis();
	CWEB_ASSERT(cur->m_state == EXEC);
	//cur->m_state = HOLD;
    //cur->swapOut();
    if(Scheduler::GetMainFiber()) {
        cur->swapOut();
    } else {
        cur->back();
    }
}

uint64_t Fiber::TotalFibers() {
	return s_fiber_count;
}

void Fiber::MainFunc() {
	Fiber::ptr cur = GetThis();
    CWEB_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        CWEB_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
            << " fiber_id = " << cur->getId()
            << std::endl
            << cweb::BacktraceToString(10);
    } catch (...) {
        cur->m_state = EXCEPT;
        CWEB_LOG_ERROR(g_logger) << "Fiber Except"
            << " fiber_id = " << cur->getId()
            << std::endl
            << cweb::BacktraceToString(10);
    }

    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->swapOut();
    // if(Scheduler::GetMainFiber()) {
    //     raw_ptr->swapOut();
    // } else {
    //     raw_ptr->back();
    // }
    CWEB_ASSERT2(false, "never reach fiber id = " + std::to_string(raw_ptr->getId()));
}

void Fiber::CallerMainFunc() {
    Fiber::ptr cur = GetThis();
    CWEB_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        CWEB_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
            << " fiber_id = " << cur->getId()
            << std::endl
            << cweb::BacktraceToString(10);
    } catch (...) {
        cur->m_state = EXCEPT;
        CWEB_LOG_ERROR(g_logger) << "Fiber Except"
            << " fiber_id = " << cur->getId()
            << std::endl
            << cweb::BacktraceToString(10);
    }

    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->back();
    // if(Scheduler::GetMainFiber()) {
    //     raw_ptr->swapOut();
    // } else {
    //     raw_ptr->back();
    // }
    CWEB_ASSERT2(false, "never reach fiber_id = " + std::to_string(raw_ptr->getId()));

}


}