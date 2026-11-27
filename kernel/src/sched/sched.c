#include "sched.h"
#include <stddef.h>
#include "gdt/gdt.h"
#include "mm/pmm.h"
#include <stdbool.h>
#include "requests/requests.h"
#include "mm/vmm.h"
void sleep_thread(void){
	for(;;)
		__asm__ volatile("sti;hlt");

}

uint64_t ticks = 0;
struct thread* current_thread = NULL;
struct thread* idle_thread = NULL;
uint64_t next_id = 0;
struct thread* thread_list = NULL;


void sched_add_thread(struct thread* t) {
    asm volatile("cli");

    if (!thread_list) {
        t->next = t;
        t->prev = t;
        thread_list = t;
    } else {
        struct thread* anchor = current_thread ? current_thread : thread_list;
        t->next = anchor->next;
        t->prev = anchor;
        anchor->next->prev = t;
        anchor->next = t;
    }

    asm volatile("sti");
}

struct thread* thread_create(void (*entry)(void),uint64_t cr3, bool isuser, uint64_t user_rsp){
	const uint64_t HHDM_OFFSET = hhdm_request.response->offset;
	uint64_t phys = pmm_alloc_page();
	if(!phys)
		return NULL;
	uint8_t* base = (uint8_t*)(phys+HHDM_OFFSET);
	struct thread* t = (struct thread*)(base +KSTACK_SIZE -sizeof(struct thread));
	memset(t, 0, sizeof(struct thread));
	t->kstack_base = base;
	t->kstack_top = ((uint64_t)t) & ~0xFULL;
	t->tid = next_id;
	next_id++;
	t->cr3 = cr3;
	t->execution_state = READY;

	t->context.rip = (uint64_t)entry;
	t->context.rflags = 0x202;


	if (isuser) {
		t->context.rsp = user_rsp;
		t->context.cs  = 0x18 | 3;
		t->context.ss  = 0x20 | 3;
	} else {
		t->context.rsp = t->kstack_top;
		t->context.cs  = 0x08;
		t->context.ss  = 0x10;
	}
	return t;

}
void sched_init(void) {
    idle_thread = thread_create(sleep_thread, read_cr3(), false, 0);
}

void sched_tick(void) {
    ticks++;
    if (!current_thread) {
        return;
    }

    // 1. Wake up any sleeping threads across the entire circular list
    if (thread_list) {
        struct thread* enumerated_thread = thread_list;
        do {
            if (enumerated_thread->execution_state == SLEEPING && enumerated_thread->wake_tick <= ticks) {
                enumerated_thread->execution_state = READY;
            }
            enumerated_thread = enumerated_thread->next;
        } while (enumerated_thread != thread_list);
    }

    // 2. If current_thread is still RUNNING and hasn't reached quantum_end_tick, keep running it
    if (current_thread != idle_thread && current_thread->execution_state == RUNNING) {
        if (ticks < current_thread->quantum_end_tick) {
            return;
        }
        current_thread->execution_state = READY;
    }

    // 3. Pick the next READY thread
    struct thread* next = NULL;
    if (thread_list) {
        struct thread* start = (current_thread != idle_thread) ? current_thread->next : thread_list;
        struct thread* iter = start;
        do {
            if (iter->execution_state == READY) {
                next = iter;
                break;
            }
            iter = iter->next;
        } while (iter != start);
    }

    if (!next) {
        next = idle_thread;
    }

    // 4. Switch to next
    next->execution_state = RUNNING;
    next->quantum_end_tick = ticks + 5;
    current_thread = next;

    gdt_set_rsp0(next->kstack_top);
    __asm__ volatile("movq %0, %%cr3" : : "r"((uint64_t)next->cr3) : "memory");
    context_switch(&(next->context));
}

