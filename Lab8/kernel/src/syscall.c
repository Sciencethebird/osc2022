#include "syscall.h"

#include <stddef.h>
#include <stdint.h>

#include "mini_uart.h"
#include "printf.h"
#include "thread.h"
#include "exception.h"
#include "mbox.h"
#include "thread.h" 
#include "io.h"
#include "timer.h"
#include "utils.h"
#include "syscall.h"
#include "vfs.h"

void syscall_handler(uint32_t syscall_number, trap_frame_t *trap_frame) {
  switch (syscall_number) {

    case SYS_GETPID:
      sys_getpid(trap_frame);
      break;
    case SYS_UART_READ:
      sys_uart_read(trap_frame);
      break;
    case SYS_UART_WRITE:
      sys_uart_write(trap_frame);
      break;

    case SYS_EXEC:
      sys_exec(trap_frame);
      break;
    case SYS_FORK:
      sys_fork(trap_frame);
      break;

    case SYS_EXIT:
      sys_exit();
      break;
    case SYS_MBOX:
      sys_mbox_call(trap_frame);
      break;
    case SYS_KILL:
      sys_kill(trap_frame);
      break;

    case SYS_LIST:
      sys_list(trap_frame);
      break;
    case SYS_OPEN:
      //printf("[syscall] open\n");
      sys_open(trap_frame);
      break;
    case SYS_CLOSE:
      //printf("[syscall] close\n");
      sys_close(trap_frame);
      break;
    case SYS_WRITE:
      //printf("[syscall] write\n");
      sys_write(trap_frame);
      break;
    case SYS_READ:
      //printf("[syscall] read\n");
      sys_read(trap_frame);
      break;
    case SYS_MKDIR:
      //printf("[syscall] mkdir\n");
      sys_mkdir(trap_frame);
      break;
    case SYS_CHDIR:
      //printf("[syscall] chdir\n");
      sys_chdir(trap_frame);
      break;
    
    case SYS_MOUNT:
      printf("[syscall] mount\n");
      sys_mount(trap_frame);
      break;
    /*
    case SYS_UMOUNT:
      sys_umount(trap_frame);
      break;
    */
    default:
      printf("[ERROR] unhandled syscall number!!!\n");
  }
}

void sys_getpid(trap_frame_t *trap_frame) {
  uint32_t pid = get_current()->pid;
  trap_frame->x[0] = pid;
}

void sys_uart_read(trap_frame_t *trap_frame) {
  //char *buf = (char *)trap_frame->x[0];
  //uint32_t size = (uint32_t)trap_frame->x[1];
  //size = uart_gets(buf, size);
  //trap_frame->x[0] = size;
  disable_uart_interrupt(); // I'm not sure why
  char *str = (char *)(trap_frame->x[0]);
  uint32_t size = (uint32_t)(trap_frame->x[1]);
  
  enable_interrupt();
  size = uart_gets(str, size);
  trap_frame->x[0] = size;
}

void sys_uart_write(trap_frame_t *trap_frame) {
  //char *buf = (char *)trap_frame->x[0];
  //uart_puts(buf);
  //trap_frame->x[0] = trap_frame->x[1];
  // print_s("uart_write called\r\n");
  disable_uart_interrupt();
  char* str = (char*) trap_frame->x[0];
  uint32_t size = (uint32_t)(trap_frame->x[1]);
  uart_write(str, size); 
}

void sys_exec(trap_frame_t *trap_frame) {
  //const char *program_name = (char *)trap_frame->x[0];
  //const char **argv = (const char **)trap_frame->x[1];
  //exec(program_name, argv);
  printf("exec called\n");
  exec();
}


void sys_fork(trap_frame_t *trap_frame) {
  //uint64_t sp = (uint64_t)trap_frame;
  //fork(sp);
  uint64_t sp = (uint64_t)trap_frame;
  printf("fork called\n");
  fork(sp);
}

void sys_exit() { exit(); }

void sys_mbox_call(trap_frame_t *trap_frame) {
      printf("mbox_call called\n");
      
      unsigned char ch = (unsigned char) trap_frame->x[0];
      unsigned int *mbox_user = (unsigned int*)(trap_frame->x[1]);

      //// manual address translation： user VA --walk--> user PA --PA2VA--> kernel PA --> 
      //unsigned int *mbox_user_PA = (unsigned int *) user_VA2PA(get_current(), (uint64_t)mbox_user_VA);
      //unsigned int *mbox_kernel_VA = (unsigned int *) PA2VA(mbox_user_PA);
      //printf("mbox ch: %d, *mbox: %d\n", ch, mbox_kernel_VA);

      int valid = mbox_call_user(ch, mbox_user);

      trap_frame->x[0] = valid;
}

void sys_kill(trap_frame_t *trap_frame){
  printf("kill called\n");
  int pid = trap_frame->x[0];
  kill(pid);
}

void sys_list(trap_frame_t *trap_frame) {
  char* pathname = (char*)trap_frame->x[0];
  vfs_list(pathname);
}

void sys_open(trap_frame_t *trap_frame) {
  
  const char *pathname = (const char *)trap_frame->x[0];
  
  int flags = (int)trap_frame->x[1];
  printf("[sys_open] called, flag: %d, dir: %s\n", flags, pathname);
  struct file *file = vfs_open(pathname, flags);
  int fd = thread_register_fd(file);
  printf("[sys_open]fd = %d\n", fd);
  trap_frame->x[0] = fd;
}

void sys_close(trap_frame_t *trap_frame) {
  int fd = (int)trap_frame->x[0];
  struct file *file = thread_get_file(fd);
  thread_clear_fd(fd);
  int result = vfs_close(file);
  trap_frame->x[0] = result;
}

void sys_write(trap_frame_t *trap_frame) {
  int fd = (int)trap_frame->x[0];
  if(fd <0){
    printf("[sys_write] error fd: %d, %x\n", fd, (unsigned int)trap_frame->x[0]);
  }
  struct file *file = thread_get_file(fd);
  const void *buf = (const void *)trap_frame->x[1];
  size_t len = (size_t)trap_frame->x[2];

  printf("[sys_write] fd: %d, len: %d\n", fd, len);
  size_t size = vfs_write(file, buf, len);
  trap_frame->x[0] = size;
}

void sys_read(trap_frame_t *trap_frame) {
  int fd = (int)trap_frame->x[0];
  struct file *file = thread_get_file(fd);
  void *buf = (void *)trap_frame->x[1];
  size_t len = (size_t)trap_frame->x[2];

  printf("[sys_read] fd: %d, len: %d\n", fd, len);
  size_t size = vfs_read(file, buf, len);
  trap_frame->x[0] = size;
}

void sys_mkdir(trap_frame_t *trap_frame) {
  const char *pathname = (const char *)trap_frame->x[0];
  int result = vfs_mkdir(pathname);
  trap_frame->x[0] = result;
}

void sys_chdir(trap_frame_t *trap_frame) {
  const char *pathname = (const char *)trap_frame->x[0];
  int result = vfs_chdir(pathname);
  trap_frame->x[0] = result;
}

void sys_mount(trap_frame_t *trap_frame) {
  const char *device = (const char *)trap_frame->x[0];
  const char *mountpoint = (const char *)trap_frame->x[1];
  const char *filesystem = (const char *)trap_frame->x[2];
  int result = vfs_mount(device, mountpoint, filesystem);
  trap_frame->x[0] = result;
}

//void sys_umount(trap_frame_t *trap_frame) {
//  const char *mountpoint = (const char *)trap_frame->x[0];
//  int result = vfs_umount(mountpoint);
//  trap_frame->x[0] = result;
//}