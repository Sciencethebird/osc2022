
#pragma once
//#include "stdint.h"
#include "utils.h"
#define CPIO_LOC 0x8000000
#define RAMFS_ADDR 0x8000000
#define CPIO_MAGIC "070701"
#define CPIO_END "TRAILER!!!"

// https://www.freebsd.org/cgi/man.cgi?query=cpio&sektion=5
typedef struct {
    char    c_magic[6];
    char    c_ino[8];
    char    c_mode[8];
    char    c_uid[8];
    char    c_gid[8];
    char    c_nlink[8];
    char    c_mtime[8];
    char    c_filesize[8];
    char    c_devmajor[8];
    char    c_devminor[8];
    char    c_rdevmajor[8];
    char    c_rdevminor[8];
    char    c_namesize[8];
    char    c_check[8];
} cpio_newc_header;

void cpio_ls();
void cpio_cat(char *pathname_to_cat);
int cpio_load_user_program(char *target_program, uint64_t target_addr);