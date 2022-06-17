#pragma once

#include <stdarg.h>
#include <stddef.h>

#define O_CREAT 1

// lab 5 system calls
int getpid();
unsigned long uart_read(char buf[], size_t size);
unsigned long uart_write(const char buf[], size_t size);
int exec(const char *name, char *const argv[]);
unsigned long fork();
void exit();
int mbox_call(unsigned char ch, unsigned int *mbox);
void kill(int pid);


int open(const char *pathname, int flags);
int close(int fd);
int write(int fd, const void *buf, int count);
int read(int fd, void *buf, int count);
void list(char* pathname);
int mkdir(const char *pathname);
int chdir(const char *pathname);
int mount(const char* device, const char* mountpoint, const char*
filesystem); int umount(const char* mountpoint);