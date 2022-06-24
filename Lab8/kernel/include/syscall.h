#pragma once

#pragma once

#include <stdint.h>

#include "thread.h"

#define SYS_GETPID 0
#define SYS_UART_READ 1
#define SYS_UART_WRITE 2
#define SYS_EXEC 3
#define SYS_FORK 4
#define SYS_EXIT 5
#define SYS_MBOX 6
#define SYS_KILL 7

#define SYS_OPEN 11
#define SYS_CLOSE 12
#define SYS_WRITE 13
#define SYS_READ 14
#define SYS_MKDIR 15
#define SYS_MOUNT 16
#define SYS_CHDIR 17
#define SYS_LSEEK64 18
#define SYS_IOCTL 19
#define SYS_SYNC 20

#define SYS_LIST 77
#define SYS_UMOUNT 88

# define SEEK_SET 0

void syscall_handler(uint32_t syscall_number, trap_frame_t *trap_frame);

void sys_getpid(trap_frame_t *trap_frame);
void sys_uart_read(trap_frame_t *trap_frame);
void sys_uart_write(trap_frame_t *trap_frame);
void sys_exec(trap_frame_t *trap_frame);
void sys_fork(trap_frame_t *trap_frame);
void sys_exit();
void sys_mbox_call(trap_frame_t *trap_frame);
void sys_kill(trap_frame_t *trap_frame);

void sys_open(trap_frame_t *trap_frame);
void sys_close(trap_frame_t *trap_frame);
void sys_write(trap_frame_t *trap_frame);
void sys_read(trap_frame_t *trap_frame);
void sys_list(trap_frame_t *trap_frame);
void sys_mkdir(trap_frame_t *trap_frame);
void sys_chdir(trap_frame_t *trap_frame);
void sys_mount(trap_frame_t *trap_frame);
void sys_umount(trap_frame_t *trap_frame);

void sys_ioctl(trap_frame_t *trap_frame);
void sys_lseek64(trap_frame_t *trap_frame);
void sys_sync(trap_frame_t *trap_frame);