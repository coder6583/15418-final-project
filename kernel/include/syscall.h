/**
 * @file    syscall.h
 *
 * @brief
 *
 * @date
 *
 * @author
 */

#ifndef _SYSCALLS_H_
#define _SYSCALLS_H_

#include <unistd.h>

void *sys_sbrk(int incr);

int sys_write(int file, char *ptr, int len);

int sys_read(int file, char *ptr, int len);

void sys_exit(int status);

int sys_get_rank(void);

#endif /* _SYSCALLS_H_ */
