/**
* @file     systick.c
*
* @brief    Controls the SysTick timer that calls pendSV every tick
*           Manages the timestamp of every thread
*
* @date     11/09/2025
*
* @author   Soma Narita <snarita@andrew.cmu.edu>
*           Josef Macera <jmacera@andrew.cmu.edu>
*/

#include <unistd.h>
#include <systick.h>
#include <syscall_thread.h>
#include <arm.h>

struct stk_reg_map {
    volatile uint32_t STK_CTRL;
    volatile uint32_t STK_LOAD;
    volatile uint32_t STK_VAL;
    volatile uint32_t STK_CALIB;
};

#define STK_BASE (struct stk_reg_map *) 0xE000E010

#define STK_CLK_FREQ 16000000 // 16MHz
#define STK_CLKSRC (1 << 2) // Select 16MHz
#define STK_TICKINT (1 << 1) // Enable interrupt
#define STK_EN (1 << 0) // Enable systick counter

#define UNUSED __attribute__((unused))

volatile uint32_t tick_cnt = 0;

void systick_init(UNUSED uint32_t frequency) {
    struct stk_reg_map *stk = STK_BASE;
    stk -> STK_LOAD = (STK_CLK_FREQ) / frequency - 1;
    stk -> STK_CTRL |= STK_CLKSRC;
    stk -> STK_CTRL |= STK_TICKINT;
    stk -> STK_CTRL |= STK_EN;

    tick_cnt = 0;

    return;

}

void systick_stop() {
    struct stk_reg_map *stk = STK_BASE;
    stk -> STK_CTRL &= ~STK_EN;
}

void systick_delay(uint32_t ticks) {
    uint32_t start = systick_get_ticks();
    while((systick_get_ticks() - start) < ticks) {
    }
    return;
}

uint32_t systick_get_ticks() {
        return tick_cnt;
}

void systick_c_handler() {
    tick_cnt += 1;
    get_current_tcb()->running_time++;
    get_current_tcb()->time++;
    if (get_current_tcb()->running_time >= get_current_tcb()->C &&
            get_current_tcb()->priority != 14) {
        get_current_tcb()->running_time = 0;
        get_current_tcb()->status = WAITING;
    } else {
        get_current_tcb()->status = RUNNABLE;
    }

    for(uint8_t i = 0; i < 14; i++) {
        tcb_t *tcb = get_tcb_at(i);
        if (tcb->enabled) {
            tcb->period_time++;
            if (tcb->period_time >= tcb->T) {
                tcb->status = RUNNABLE;
                tcb->period_time = 0;
                tcb->running_time = 0;
            }
        }
    }
    pend_pendsv();
}
