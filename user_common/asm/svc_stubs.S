/**
 * @file   swi_stubs.S
 *
 * @brief  Stub functions for newlib and 349libc syscalls
 *
 * @date
 * @author
 */

.cpu cortex-m4
.syntax unified
.section .swi_stub
.thumb

#include "../../kernel/include/svc_num.h"

.global _start
_start:
  svc #SVC_SCHD_START
  bx lr

.global _sbrk
_sbrk:
  svc #SVC_SBRK
  bx lr

.global _write
_write:
  svc #SVC_WRITE
  bx lr

.global _close
_close:
  svc #SVC_CLOSE
  bx lr

.global _fstat
_fstat:
  svc #SVC_FSTAT
  bx lr

.global _isatty
_isatty:
  svc #SVC_ISATTY
  bx lr

.global _lseek
_lseek:
  svc #SVC_LSEEK
  bx lr

.global _read
_read:
  svc #SVC_READ
  bx lr

.global _gettimeofday
_gettimeofday:
 svc #SVC_TIME
 bx lr 
.global _times
_times:
 svc #SVC_TIME
 bx lr

@ .global _getpid
@ _getpid:
@   svc #SVC_GET_PID
@   bx lr

@ .global _kill
@ _kill:
@   svc #SVC_KILL
@   bx lr

.global _exit
_exit:
  svc #SVC_EXIT
  bx lr

.global thread_create
thread_create:
  svc #SVC_THR_CREATE
  bx lr

.global thread_init
thread_init:
  svc #SVC_THR_INIT
  bx lr

.global get_priority
get_priority:
  svc #SVC_PRIORITY
  bx lr

.global thread_time
thread_time:
  svc #SVC_THR_TIME
  bx lr

.global thread_kill
thread_kill:
  svc #SVC_THR_KILL
  bx lr

.global get_time
get_time:
  svc #SVC_TIME
  bx lr

.global scheduler_start
scheduler_start:
  svc #SVC_SCHD_START
  bx lr

.global wait_until_next_period
wait_until_next_period:
  svc #SVC_WAIT
  bx lr

.global motor_set
motor_set:
  svc #SVC_SET_MOTOR
  bx lr

.global register_encoder_callback
register_encoder_callback:
  svc #SVC_REG_ENC_CALLBACK
  bx lr

.global lcd_clear_scr
lcd_clear_scr:
  svc #SVC_LCD_CLEAR
  bx lr

.global lcd_set_cursor_pos
lcd_set_cursor_pos:
  svc #SVC_LCD_SET_CURSOR
  bx lr

.global lcd_print_str
lcd_print_str:
  svc #SVC_LCD_PRINT
  bx lr

.global lcd_init
lcd_init:
  svc #SVC_LCD_INIT
  bx lr

.global spi_init
spi_init:
  svc #SVC_SPI_INIT
  bx lr

.global spi_transmit
spi_transmit:
  svc #SVC_SPI_TRANSMIT
  bx lr

.global spi_receive
spi_receive:
  svc #SVC_SPI_RECEIVE
  bx lr

.global sleep
sleep:
  svc #SVC_SLEEP
  bx lr

.global spi_rx_ready
spi_rx_ready:
  svc #SVC_SPI_RX_READY
  bx lr

.global spi_rx_dequeue
spi_rx_dequeue:
  svc #SVC_SPI_RX_DEQUEUE
  bx lr
