#include "printf.h"
#include "syscall.h"
#include "utils.h"

#define O_CREAT 00000100

int main(int argc, char** argv) {

  printf("Program: pid: %d\n",  getpid());

  list("/");

  // mkdir test
  mkdir("/test_dir");
  list("/");

  // create file test
  int fd = open("/test_dir/test_file", O_CREAT);
  printf("[open] fd = %d\n", fd);
  list("/test_dir");

  // write, close test
  char msg[20] = "0123456789";
  write(fd, (const void *)msg, 10);
  close(fd);

  // read test
  char buf[200];
  fd = open("/test_dir/test_file", 0);
  read(fd, (void*) buf, 4);
  printf("read result: %s\n", buf);

  // chdir test
  chdir("/test_dir");
  list(".");

  chdir("..");
  list(".");

  // initramfs test
  fd = open("/initramfs/happy", 0);
  read(fd, (void*) buf, 100);
  printf("read result: %s\n", buf);

  // write std resutl
  write(1, buf, 100);
  return 0;
}