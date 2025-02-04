#include <stdint.h>
#include "string.h"
#include "printf.h"
#include "io.h"
#include "cpio.h"
#include "utils.h"
#include "mini_uart.h"
#include "vfs.h"

void cpio_ls() {
  unsigned long long ptr = RAMFS_ADDR;
  cpio_newc_header *header;
  char *pathname;

  while (1) {
    header = (cpio_newc_header *)ptr;
    unsigned long long namesize = hex2int(header->c_namesize, 8);
    unsigned long long filesize = hex2int(header->c_filesize, 8);

    ptr += sizeof(cpio_newc_header);
    pathname = (char *)ptr;

    // the end is indicated by a special record with pathname "TRAILER!!!"
    if (strcmp(pathname, CPIO_END) == 0) break;
    printf(pathname);
    printf(" ");

    ptr = align_up(ptr + namesize, 4);
    ptr = align_up(ptr + filesize, 4);
  }
  printf("\n");
}

void cpio_cat(char *pathname_to_cat) {
  unsigned long long ptr = RAMFS_ADDR;
  cpio_newc_header *header;
  char *pathname;

  while (1) {
    header = (cpio_newc_header *)ptr;
    unsigned long long namesize = hex2int(header->c_namesize, 8);
    unsigned long long filesize = hex2int(header->c_filesize, 8);

    ptr += sizeof(cpio_newc_header);
    pathname = (char *)ptr;
    // the end is indicated by a special record with pathname "TRAILER!!!"
    if (strcmp(pathname, CPIO_END) == 0) break;

    ptr = align_up(ptr + namesize, 4);
    if (strcmp(pathname, pathname_to_cat) == 0) {
      char *content = (char *)ptr;
      for (unsigned long long i = 0; i < filesize; i++) {
        if (content[i] == '\n') print_c('\r');
        print_c(content[i]);
      }
      print_s("\n");
      return;
    }
    ptr = align_up(ptr + filesize, 4);
  }
  print_s("No such file\n");
}

int cpio_load_user_program(char *target_program, uint64_t target_addr) {
  unsigned long long ptr = RAMFS_ADDR;
  cpio_newc_header *header;
  char *pathname;

  while (1) {
    header = (cpio_newc_header *)ptr;
    unsigned long long namesize = hex2int(header->c_namesize, 8);
    unsigned long long filesize = hex2int(header->c_filesize, 8);

    ptr += sizeof(cpio_newc_header);
    pathname = (char *)ptr;
    // the end is indicated by a special record with pathname "TRAILER!!!"
    if (strcmp(pathname, CPIO_END) == 0) break;

    ptr = align_up(ptr + namesize, 4);
    if (strcmp(pathname, target_program) == 0) {
      char *content = (char *)ptr;
      char *target_content = (char *)target_addr;
      for (unsigned long long i = 0; i < filesize; i++) {
        target_content[i] = content[i];
      }
      return filesize;
    }
    ptr = align_up(ptr + filesize, 4);
  }
  printf("No such file\n");
  return 0;
}

void cpio_populate_rootfs() {
  printf("===== cpio_populate_rootfs =====\n");
  unsigned long long ptr = RAMFS_ADDR;
  cpio_newc_header *header;
  char *pathname;

  while (1) {
    header = (cpio_newc_header *)ptr;
    unsigned long long namesize = hex2int(header->c_namesize, 8);
    unsigned long long filesize = hex2int(header->c_filesize, 8);

    ptr += sizeof(cpio_newc_header);
    pathname = (char *)ptr;
    // the end is indicated by a special record with pathname "TRAILER!!!"
    if (strcmp(pathname, CPIO_END) == 0) break;
    printf("pathname: %s, size: %d\n", pathname, filesize);

    ptr = align_up(ptr + namesize, 4);
    if (filesize > 0) {
      struct file *file = vfs_open(pathname, O_CREAT);
      if (file) {
        vfs_write(file, (const char *)ptr, filesize);
        vfs_close(file);
      }
    }
    ptr = align_up(ptr + filesize, 4);
  }
}