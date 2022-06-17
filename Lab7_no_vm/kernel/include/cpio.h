
#pragma once
//#include "stdint.h"
#include "utils.h"
#include "vfs.h"
//#define CPIO_LOC 0x8000000
#define RAMFS_ADDR (0x8000000)
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
void cpio_populate_rootfs();
//void* cpio_open(char *pathname);

struct cpiofs_file {
  size_t size;
  char* file_location;
};

// for cpiofs file system
#define MAX_FILES_IN_CPIO 100
struct cpiofs_fentry {
  char name[20];
  FILE_TYPE type; // all FILE_REGULAR???
  struct vnode* vnode;
  struct vnode* parent_vnode;
  struct cpiofs_fentry* child[MAX_FILES_IN_CPIO];
  struct cpiofs_file* file;
};

struct vnode_operations* cpiofs_v_ops;
struct file_operations* cpiofs_f_ops;

void cpiofs_init();
void cpiofs_set_fentry(struct cpiofs_fentry* fentry, const char* component_name,
                      struct cpiofs_file* file, FILE_TYPE type, struct vnode* vnode);
int cpiofs_setup_mount(struct filesystem* fs, struct mount* mount);
int cpiofs_lookup(struct vnode* dir_node, struct vnode** target,
                 const char* component_name);
int cpiofs_create(struct vnode* dir_node, struct vnode** target,
                 const char* component_name, FILE_TYPE type);
int _cpiofs_create(struct vnode* dir_node, const char* component_name, 
                  struct cpiofs_file* file, FILE_TYPE type);
int cpiofs_set_parent(struct vnode* child_node, struct vnode* parent_vnode);
int cpiofs_write(struct file* file, const void* buf, size_t len);
int cpiofs_read(struct file* file, void* buf, size_t len);
void cpiofs_list(struct vnode* dir);