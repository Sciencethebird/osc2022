#include <stdint.h>
#include "string.h"
#include "alloc.h"
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
  printf("[cpio_load_user_program] No such file\n");
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


// cpiofs file system
void cpiofs_init() {
  cpiofs_v_ops =
      (struct vnode_operations*)malloc(sizeof(struct vnode_operations));
  cpiofs_v_ops->lookup = cpiofs_lookup;
  cpiofs_v_ops->create = cpiofs_create;
  cpiofs_v_ops->set_parent = cpiofs_set_parent;
  cpiofs_f_ops = (struct file_operations*)malloc(sizeof(struct file_operations));
  cpiofs_f_ops->write = cpiofs_write;
  cpiofs_f_ops->read = cpiofs_read;
  cpiofs_f_ops->list = cpiofs_list;
}

void cpiofs_set_fentry(struct cpiofs_fentry* fentry, const char* component_name,
                      struct cpiofs_file* file, FILE_TYPE type, struct vnode* vnode) {
  strcpy(fentry->name, component_name);
  fentry->vnode = vnode;
  fentry->type = type;
  fentry->file = file;
  if (fentry->type == FILE_DIRECTORY) {
    for (int i = 0; i < MAX_FILES_IN_CPIO; ++i) {
      fentry->child[i] =
          (struct cpiofs_fentry*)malloc(sizeof(struct cpiofs_fentry));
      fentry->child[i]->name[0] = 0;
      fentry->child[i]->type = FILE_NONE;
      fentry->child[i]->parent_vnode = vnode;
    }
    fentry->file->size = 0;
    
  }
}

int cpiofs_setup_mount(struct filesystem* fs, struct mount* mount) {
  // setup cpio root node
  struct cpiofs_fentry* root_fentry =
      (struct cpiofs_fentry*)malloc(sizeof(struct cpiofs_fentry));
  struct vnode* vnode = mount->root;

  vnode->mount = mount;
  vnode->v_ops = cpiofs_v_ops;
  vnode->f_ops = cpiofs_f_ops;
  vnode->internal = (void*)root_fentry;

  root_fentry->parent_vnode = 0;
  cpiofs_set_fentry(root_fentry, mount->mount_entry, 0, FILE_DIRECTORY, vnode);
  mount->fs = fs;
  mount->root = vnode;

  // load cpio file names into cpiofs_fentry
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
    printf("[cpiofs_setup_mount] pathname: %s, size: %d\n", pathname, filesize);

    
    ptr = align_up(ptr + namesize, 4);

    // get cpio file size and file location ptr.
    struct cpiofs_file* file = (struct cpiofs_file*)malloc(sizeof(struct cpiofs_file));
    file->size = filesize;
    file->file_location = (char*) ptr;
    _cpiofs_create(mount->root, pathname, file, FILE_REGULAR);

    ptr = align_up(ptr + filesize, 4);
  }

  return 1;
}

int cpiofs_lookup(struct vnode* dir_node, struct vnode** target,
                 const char* component_name) {
  //printf("[cpiofs_lookup] %s\n", component_name);
  struct cpiofs_fentry* fentry = (struct cpiofs_fentry*)dir_node->internal;
  if (fentry->type != FILE_DIRECTORY) return 0;

  if (!strcmp(component_name, ".")) {
    //printf("[cpiofs_lookup] .\n");
    *target = fentry->vnode;
    return 1;
  }
  if (!strcmp(component_name, "..")) {
    //printf("[cpiofs_lookup] ..\n");
    if (!fentry->parent_vnode) return 0;
    *target = fentry->parent_vnode;
    return 1;
  }

  for (int i = 0; i < MAX_FILES_IN_CPIO; i++) {
    
    fentry = ((struct cpiofs_fentry*)dir_node->internal)->child[i];
    //printf("[cpiofs_lookup] [%s]\n", fentry->name);
    if (!strcmp(fentry->name, component_name)) {
      *target = fentry->vnode;
      return 1;
    }
  }
  return 0;
}

int cpiofs_create(struct vnode* dir_node, struct vnode** target,
                 const char* component_name, FILE_TYPE type) {

  printf("[cpiofs_create] forbidden operation: creating a file on a read-only filesystem.\n");
  return -1;
}

// "private" function for init cpiofs
int _cpiofs_create(struct vnode* dir_node, const char* component_name, 
                  struct cpiofs_file* file, FILE_TYPE type) {

  for (int i = 0; i < MAX_FILES_IN_CPIO; i++) {
    struct cpiofs_fentry* fentry =
        ((struct cpiofs_fentry*)dir_node->internal)->child[i];
    if (fentry->type == FILE_NONE) {
      struct vnode* vnode = (struct vnode*)malloc(sizeof(struct vnode));
      vnode->mount = 0;
      vnode->v_ops = dir_node->v_ops;
      vnode->f_ops = dir_node->f_ops;
      vnode->internal = fentry;
      cpiofs_set_fentry(fentry, component_name, file, type, vnode);
      return 1;
    }
  }
  return -1;
}

int cpiofs_set_parent(struct vnode* child_node, struct vnode* parent_vnode) {
  struct cpiofs_fentry* fentry = (struct cpiofs_fentry*)child_node->internal;
  fentry->parent_vnode = parent_vnode;
  return 1;
}

int cpiofs_write(struct file* file, const void* buf, size_t len) {

  printf("[cpiofs_write] forbidden operation: writing a file on a read-only filesystem!\n");
  return -1;
}

int cpiofs_read(struct file* file, void* buf, size_t len) {
  size_t read_len = 0;
  struct cpiofs_fentry* fentry = (struct cpiofs_fentry*)file->vnode->internal;
  for (size_t i = 0; i < len; i++) {
    ((char*)buf)[i] = fentry->file->file_location[file->f_pos++];
    read_len++;
    if (read_len == fentry->file->size) {
      break;
    }
  }

  return read_len;
}

void cpiofs_list(struct vnode* dir_node) {
  printf("[cpiofs_list]: listing dir: %s\n", ((struct cpiofs_fentry*)dir_node->internal)->name);
  for (int i = 0; i < MAX_FILES_IN_CPIO; i++) {
    struct cpiofs_fentry* fentry =
        ((struct cpiofs_fentry*)dir_node->internal)->child[i];
    if (fentry->type != FILE_NONE) {
      printf(" [type]: %d, [name]: %s\n", fentry->type, fentry->name);
    }
  }
  printf("\n");
}
