/**
 * @file syscall.c 
 *
 * @brief basic immplementations of syscalls (sbrk, write, read, exit)
 *
 * @date 11/1/2025
 *
 * @author Josef Macera <jmacera@andrew.cmu.edu>
 */

#include <unistd.h>
#include <syscall.h>
#include <uart.h>
#include <stdio.h>
#include <systick.h>
#include <arm.h>
#include "printk.h"
#include <gpio.h>

extern char __heap_low;
extern char __heap_top;
#define UNUSED __attribute__((unused))

static char *current_break = 0;

// increase the heap limit break
void *sys_sbrk(UNUSED int incr){
    if (current_break == 0)
        current_break = &__heap_low;

    char *new = current_break + incr;
    
    if (new > &__heap_top)
        return (void*) -1;

    current_break = new;
    return current_break;
}

// write len bytes to a file
int sys_write(UNUSED int file, UNUSED char *ptr, UNUSED int len){
    if (file != 1) return -1;
    int r = 0;
    for(int i = 0; i < len; i++) {
        while(uart_put_byte(ptr[r]) != 0);
        r++;
    }
    return r;
}

// read len bytes from a file and put them into ptr
int sys_read(UNUSED int file, UNUSED char *ptr, UNUSED int len){
    if (file != 0) return -1;
    char c;
    int r = 0;
    while (r < len) {
        int i	= uart_get_byte(&c);
        if (i == -1) {}
        else if (c == 4) return r;
        else if (c == '\r') {
            ptr[r] = c;
            while(uart_put_byte(c) != 0);
            while(uart_put_byte('\n') != 0);
            systick_delay(1);
            return r;
        }
        else if (c == '\b') {
            while(uart_put_byte(c) != 0);
            c = ' ';
            while(uart_put_byte(c) != 0);
            c = '\b';
            while(uart_put_byte(c) != 0);
            systick_delay(1);
            if (r != 0)
                ptr[r--] = '\0';
            continue;
        }
        else {
            ptr[r] = c;
            while(uart_put_byte(c) != 0);
            r++;
        }
    }
    return r;
}


// called on sys_exit
void sys_exit(UNUSED int status){
    printk("%d", status);
    uart_flush();
//    save_interrupt_state_and_disable();
    while (1);
}

int sys_get_rank(void) {
    gpio_init(GPIO_A, 0, MODE_INPUT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_PULL_DOWN, ALT0);
    return gpio_read(GPIO_A, 0);
}
