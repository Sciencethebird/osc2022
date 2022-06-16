#include "syscall.h"

int getpid() {
  asm volatile("mov x8, #0");
  asm volatile("svc 0");
  return;
}

unsigned long uart_read(char buf[], size_t size) {
  asm volatile("mov x8, #1");
  asm volatile("svc 0");
  return;
}

unsigned long uart_write(const char buf[], size_t size) {
  asm volatile("mov x8, #2");
  asm volatile("svc 0");
  return;
}

int exec(const char *name, char *const argv[]) {
  asm volatile("mov x8, #3");
  asm volatile("svc 0");
  return;
}

unsigned long fork() {
  asm volatile("mov x8, #4");
  asm volatile("svc 0");
  return;
}

void exit() {
  asm volatile("mov x8, #5");
  asm volatile("svc 0");
  return;
}

int mbox_call(unsigned char ch, unsigned int *mbox){
  asm volatile("mov x8, #6");
  asm volatile("svc 0");
  return;
}

void kill(int pid){
  asm volatile("mov x8, #7");
  asm volatile("svc 0");
  return;
}


void list(char* pathname) {
  asm volatile("mov x8, #77");
  asm volatile("svc 0");
  return;
}



int open(const char *pathname, int flags) {
  asm volatile("mov x8, #11");
  asm volatile("svc 0");
  return;
}


int close(int fd) {
  asm volatile("mov x8, #12");
  asm volatile("svc 0");
  return;
}

int write(int fd, const void *buf, int count) {
  asm volatile("mov x8, #13");
  asm volatile("svc 0");
  return;
}

int read(int fd, void *buf, int count) {
  asm volatile("mov x8, #14");
  asm volatile("svc 0");
  return;
}

int mkdir(const char *pathname) {
  asm volatile("mov x8, #15");
  asm volatile("svc 0");
  return;
}
/*

int chdir(const char *pathname) {
  asm volatile("mov x8, #0");
  asm volatile("svc 0");
  return;
}

int mount(const char *device, const char *mountpoint, const char *filesystem) {
  asm volatile("mov x8, #0");
  asm volatile("svc 0");
  return;
}

int umount(const char *mountpoint) {
  asm volatile("mov x8, #0");
  asm volatile("svc 0");
  return;
}
*/