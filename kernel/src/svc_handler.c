/**
 * @file svc_handler.c
 *
 * @brief SVC handler function
 *
 * @date 11/1/2025
 *
 * @author Josef Macera <jmacera@andrew.cmu.edu>
 */

#include <stdint.h>
#include <debug.h>
#include <svc_num.h>
#include <syscall.h>
#include <systick.h>
#include <syscall_thread.h>
#include <motor_driver.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <lcd_driver.h>
#include <spi.h>
#define UNUSED __attribute__((unused))

// case on svc calls and map them to corresponding syscalls
void svc_c_handler(uint32_t *svc_addr, uint32_t *stack_frame ) {
    uint32_t ins = *(uint16_t*)svc_addr;
    uint8_t svc_num = ins & 0xFF;

    uint32_t r0 = stack_frame[0];     // argument 1
    uint32_t r1 = stack_frame[1];     // argument 2
    uint32_t r2 = stack_frame[2];     // argument 3
    uint32_t r3 = stack_frame[3];
    uint32_t r4 = stack_frame[8];

    uint32_t r = -1;
    switch (svc_num) {
    case SVC_SBRK:
        r = (uint32_t)sys_sbrk(r0);
        break;
    case SVC_WRITE:
        r = (uint32_t)sys_write(r0, (char*)r1, r2);
        break;
    case SVC_READ:
        r = (uint32_t)sys_read(r0, (char*)r1, r2);
        break;
    case SVC_EXIT:
        sys_exit(r0);
        return;
    case SVC_THR_INIT:
        r = (uint32_t)sys_thread_init(r0, r1, (void*)r2, r3);
        break;
    case SVC_THR_CREATE:
        r = (uint32_t)sys_thread_create((void*)r0, r1, r2, r3, (void*)r4);
        break;
    case SVC_SCHD_START:
        r = (uint32_t)sys_scheduler_start(r0);
        break;
    case SVC_WAIT:
        sys_wait_until_next_period();
        break;
    case SVC_TIME:
        r = (uint32_t)sys_get_time();
        break;
    case SVC_PRIORITY:
        r = (uint32_t)sys_get_priority();
        break;
    case SVC_THR_TIME:
        r = (uint32_t)sys_thread_time();
        break;
    case SVC_THR_KILL:
        sys_thread_kill();
        return;
    case SVC_SET_MOTOR:
	    sys_motor_set((enum motor_mapping) r0, (uint32_t)r1, (direction_t)r2);
	    return;
    case SVC_REG_ENC_CALLBACK:
	    sys_register_encoder_callback((uint32_t)r0, (void*)r1);
	    return;
    case SVC_LCD_CLEAR:
	    lcd_clear();
	    return;
    case SVC_LCD_SET_CURSOR:
	    lcd_set_cursor((uint8_t) r0, (uint8_t) r1);
	    return;
    case SVC_LCD_PRINT:
	    lcd_print((char *)r0);
	    return;
    case SVC_LCD_INIT:
	    lcd_driver_init();
	    return;
    case SVC_SPI_INIT:
        sys_spi_init((uint8_t) r0);
        return;
    case SVC_SPI_TRANSMIT:
        sys_spi_transmit((void*)r0, (uint32_t)r1);
        return;
    case SVC_SPI_RECEIVE:
        sys_spi_receive((void*)r0, (uint32_t)r1);
        return;
    case SVC_SLEEP:
	systick_delay(r0);
	return;
    default:
        return;
    }
    stack_frame[0] = r;
    return;
}
