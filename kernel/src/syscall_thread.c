/** @file   syscall_thread.c
 *
 *  @brief  Implements system calls for thread-related functions. Uses RMS scheduling
 *
 *  @date   11/09/2025
 *
 *  @author Soma Narita <snarita@andrew.cmu.edu>
 *          Josef Macera <jmacera@andrew.cmu.edu>
 */

#include <stdint.h>
#include <printk.h>
#include "syscall_thread.h"
#include "syscall_mutex.h"
#include <arm.h>
#include <systick.h>
#include <syscall.h>

/** @brief      Initial XPSR value, all 0s except thumb bit. */
#define XPSR_INIT 0x1000000

/** @brief Interrupt return code to user mode using PSP.*/
#define LR_RETURN_TO_USER_PSP 0xFFFFFFFD
/** @brief Interrupt return code to kernel mode using MSP.*/
#define LR_RETURN_TO_KERNEL_MSP 0xFFFFFFF1

#define USER_STACK 0
#define KERNEL_STACK 1

/**
 * @brief      Heap high and low pointers.
 */
//@{
extern char 
    __thread_u_stacks_low,
    __thread_u_stacks_top,
    __thread_k_stacks_low,
    __thread_k_stacks_top;
//@}

/**
 * @brief      Precalculated values for UB test.
 */
uint32_t ub_table[] = {
    0, 10000, 8284, 7798, 7568,
    7435, 7348, 7286, 7241, 7205,
    7177, 7155, 7136, 7119, 7106,
    7094, 7083, 7075, 7066, 7059,
    7052, 7047, 7042, 7037, 7033,
    7028, 7025, 7021, 7018, 7015,
    7012, 7009
};

/**
 * @struct user_stack_frame
 *
 * @brief  Stack frame upon exception.
 */
typedef struct {
    uint32_t r0;   /** @brief Register value for r0 */
    uint32_t r1;   /** @brief Register value for r1 */
    uint32_t r2;   /** @brief Register value for r2 */
    uint32_t r3;   /** @brief Register value for r3 */
    uint32_t r12;  /** @brief Register value for r12 */
    uint32_t lr;   /** @brief Register value for lr*/
    uint32_t pc;   /** @brief Register value for pc */
    uint32_t xPSR; /** @brief Register value for xPSR */
} interrupt_stack_frame;

typedef struct {
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t psp;
    uint32_t lr;
} context_stack_frame;


uint8_t num_threads_created;
uint8_t num_threads_max;
tcb_t tcb_list[16];
tcb_t* current_tcb;
void* tcb_idle_fn;
uint32_t thread_stack_size;
int is_schedulable();

extern void thread_kill();

tcb_t *get_next_tcb() {
    if (num_threads_created == 0) return &tcb_list[15];
    for (int i = 0; i < 16; i++) {
        if (tcb_list[i].enabled && tcb_list[i].status == RUNNABLE) {
            return &tcb_list[i];
        }
    }
    return &tcb_list[14];
}

void default_idle_thread() {
    while(1) wait_for_interrupt();
}

void thread_wrapper(void *t(void* args0), void *args0) {
    t(args0);
    thread_kill();
}

// PendSV C Handler used for switching contexts
void *pendsv_c_handler(void *context_ptr, void *psp_ptr) {
    if (get_svc_status()) {
        current_tcb->used_stack = KERNEL_STACK;
    } else {
        current_tcb->used_stack = USER_STACK;
    }
    current_tcb->user_stack = psp_ptr;
    // Context ptr is always in kernel mode
    current_tcb->kernel_stack = context_ptr;

    tcb_t *next_tcb = get_next_tcb();
    next_tcb -> status = RUNNING; 
    current_tcb = next_tcb;
    set_svc_status(next_tcb -> used_stack);
    return next_tcb -> kernel_stack; 
}

// Initializes global variables
// Creates the default and idle threads
int sys_thread_init(
    uint32_t max_threads,
    uint32_t stack_size,
    void *idle_fn,
    uint32_t max_mutexes
){
    (void) max_mutexes;
    num_threads_max = max_threads;	
    num_threads_created = 0;
    if (idle_fn == NULL) idle_fn = &default_idle_thread;
    tcb_idle_fn = idle_fn;
    if (max_threads > 14) {
        printk("ERROR: max_threads must be 14 or below.\n");
        return -1;
    }
    if (stack_size > 256) {
        printk("ERROR: stack_size must be 256 or below.\n");
        return -1;
    }

    // Set all TCBs as disabled
    for (unsigned int i = 0; i < 16; i++) {
        tcb_list[i].enabled = 0;
    }
    thread_stack_size = stack_size;
    current_tcb = &(tcb_list[15]);

    // Create idle thread
    sys_thread_create(idle_fn, 14, 0, 1, NULL);
    // Create default thread
    // Put unsigned integer max value as C and T
    sys_thread_create((void*)NULL, 15, 0, ~0, NULL);
    return 0;
}

// Creates a thread and initializes the stack frame
int sys_thread_create(
    void *fn,
    uint32_t prio,
    uint32_t C,
    uint32_t T,
    void *vargp
){
    if (prio > 15) {
        printk("ERROR: Priority must be 15 or below.\n");
        return -1;
    }
    if (num_threads_created == num_threads_max) {
        printk("ERROR:max thread limit reached.\n");
        return -1;
    }
    if (prio != 14 && prio != 15)
        num_threads_created++;
    tcb_t *tcb = &(tcb_list[prio]);
    
    tcb -> priority = prio;
    tcb -> user_stack = (void*)(&__thread_u_stacks_top - prio * thread_stack_size * 4);
    tcb -> kernel_stack = (void*)(&__thread_k_stacks_top - prio * thread_stack_size * 4);	
    tcb -> T = T;
    tcb -> C = C;
    tcb -> status = RUNNABLE;
    tcb -> enabled = 1;
    tcb -> used_stack = USER_STACK;
    tcb -> time = 0;

    if (is_schedulable() == -1) {
        tcb->enabled = 0;
        num_threads_created--;
        return -1;
    }
    interrupt_stack_frame *stack_frame = tcb -> user_stack - 
                        sizeof(interrupt_stack_frame);
    stack_frame -> r0 = (uint32_t)fn;
    stack_frame -> r1 = (uint32_t) vargp;
    stack_frame -> r2 = 0;
    stack_frame -> r3 = 0;
    stack_frame -> r12 = 0;
    stack_frame -> lr = (uint32_t)&thread_kill;
    stack_frame -> pc = (uint32_t)&thread_wrapper;
    stack_frame -> xPSR = XPSR_INIT;

    context_stack_frame *init_ctx = tcb -> kernel_stack - 
                            sizeof(context_stack_frame);
    init_ctx -> r4 = 0;
    init_ctx -> r5 = 0;
    init_ctx -> r6 = 0;
    init_ctx -> r7 = 0;
    init_ctx -> r8 = 0;
    init_ctx -> r9 = 0;
    init_ctx -> r10 = 0;
    init_ctx -> r11 = 0;
    init_ctx -> lr = LR_RETURN_TO_USER_PSP;
    init_ctx -> psp = (uint32_t)stack_frame;

    tcb -> user_stack = stack_frame;
    tcb -> kernel_stack = init_ctx;
    return 0;
}

tcb_t *get_current_tcb() {
    return current_tcb;
}

tcb_t *get_tcb_at(uint8_t i) {
    return &tcb_list[i];
}

//@brief checks if the current pool of threads is schedulable
//@return 0 if yes, -1 if no
int is_schedulable() {
    // run the ub test
    uint32_t s = 0;
    uint8_t num_threads = 0;
    
    for (uint8_t i = 0; i < 14; i++)
        if (tcb_list[i].enabled) {
            s += (10000 * tcb_list[i].C) / (tcb_list[i].T);
            num_threads++;
        }
    if (s <= ub_table[num_threads]) return 0;
    return -1;
    
}

int sys_scheduler_start( uint32_t frequency ){
    if (is_schedulable() == 0) {
        systick_init(frequency);
        pend_pendsv();
        return 0;
    } 
    return -1;
}

uint32_t sys_get_priority(){
    return current_tcb->priority;
}

uint32_t sys_get_time(){
    return systick_get_ticks();
}

uint32_t sys_thread_time() {
    return current_tcb->time;
}

void sys_thread_kill() {
    num_threads_created--;
    // Not idle and not default
    if (current_tcb->priority < 14) {
        current_tcb->enabled = 0; // disableddd
    } else if (current_tcb->priority == 14) {
        current_tcb->enabled = 1;
        ((interrupt_stack_frame *)(current_tcb -> user_stack)) -> pc = (uint32_t)&default_idle_thread;
        pend_pendsv();
        return;
    } else if (current_tcb->priority == 15) {
        sys_exit(0);
        return;
    }

    char remaining_pool_size = 0;
    for(uint8_t i = 0; i < 14; i++)
        if(tcb_list[i].enabled) {
            remaining_pool_size++;
            break;
        }

    if (remaining_pool_size != 0) 
        pend_pendsv();
    else {
        systick_stop();
        get_tcb_at(14)->enabled = 0;
        pend_pendsv();
    }
}

void sys_wait_until_next_period() {
    if (current_tcb->priority == 14) return;
    current_tcb->status = WAITING;
    pend_pendsv();
}

kmutex_t *sys_mutex_init( uint32_t max_prio ) {
    (void) max_prio;
    return NULL;
}

void sys_mutex_lock( kmutex_t *mutex ) {
    (void) mutex;
}

void sys_mutex_unlock( kmutex_t *mutex ) {
    (void) mutex;
}
