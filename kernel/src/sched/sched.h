#ifndef SCHED_H
#define SCHED_H
#include "cpu/context/context.h"
enum exec_state {
	READY,
	RUNNING,
	BLOCKED,
	SLEEPING,
	TERMINATED,
};
///The amount of physical memory to allocate each time when spawning a new thread for its kernel stack
#define KSTACK_SIZE 4096 

struct thread {
	struct thread_context context;
	enum exec_state execution_state;
	///Thread ID
	uint64_t tid; 
	uint64_t cr3;
	///kstack_base + KSTACK_SIZE
	uint64_t kstack_top;
	uint64_t wake_tick;
	uint64_t quantum_end_tick;
	void* kstack_base;
	struct thread* next;
	struct thread* prev;
};

void sched_tick();
;
;
;
;
;
;
;

#endif
