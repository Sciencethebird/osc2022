
#include "mini_uart.h"
#include "printf.h"
#include "gpio.h"
#include "device.h"
#include "vfs.h"
#include "alloc.h"
#include "string.h"

/**********************************************/
/*              uart file system              */
/**********************************************/

void root_init() {
  device_v_ops =
      (struct vnode_operations*)malloc(sizeof(struct vnode_operations));
  device_v_ops->lookup = device_lookup;
  device_v_ops->create = device_create;
  device_v_ops->set_parent = device_set_parent;
  device_f_ops = (struct file_operations*)malloc(sizeof(struct file_operations));
  device_f_ops->write = device_write;
  device_f_ops->read = device_read;
  device_f_ops->list = device_list;
}

void uartfs_init() {
  uartfs_v_ops =
      (struct vnode_operations*)malloc(sizeof(struct vnode_operations));
  uartfs_v_ops->lookup = device_lookup;
  uartfs_v_ops->create = device_create;
  uartfs_v_ops->set_parent = device_set_parent;
  uartfs_f_ops = (struct file_operations*)malloc(sizeof(struct file_operations));
  uartfs_f_ops->write = uartfs_write;
  uartfs_f_ops->read = uartfs_read;
  uartfs_f_ops->list = device_list;
}

void device_init() {
  root_init();
  uartfs_init();
}

void device_set_fentry(struct device_fentry* fentry, const char* component_name,
                      struct vnode* vnode, DEV_TYPE type) {
  strcpy(fentry->name, component_name);
  fentry->vnode = vnode;
  fentry->type = type;

  if (fentry->type == DEV_ROOT) {
    for (int i = 0; i < MAX_DEVICE_IN_DIR; ++i) {
      fentry->child[i] =
          (struct device_fentry*)malloc(sizeof(struct device_fentry));
      fentry->child[i]->name[0] = 0;
      fentry->child[i]->type = DEV_NONE;
      fentry->child[i]->parent_vnode = vnode;
    }
  } else {
    for (int i = 0; i < MAX_DEVICE_IN_DIR; ++i) {
      fentry->child[i] = 0;
      fentry->child[i]->name[0] = 0;
      fentry->child[i]->type = DEV_NONE;
      fentry->child[i]->parent_vnode = vnode;
    }
  }
}

int device_setup_mount(struct filesystem* fs, struct mount* mount) {
  // setup cpio root node
  struct device_fentry* root_fentry =
      (struct device_fentry*)malloc(sizeof(struct device_fentry));
  struct vnode* vnode = mount->root;

  vnode->mount = mount;
  vnode->v_ops = device_v_ops;
  vnode->f_ops = device_f_ops;
  vnode->internal = (void*)root_fentry;

  root_fentry->parent_vnode = 0;
  device_set_fentry(root_fentry, mount->mount_entry, vnode, DEV_ROOT);
  mount->fs = fs;
  mount->root = vnode;
  
  // create device vnode
  _device_create(mount->root, "uart", DEV_UART);

  return 1;
}

int device_lookup(struct vnode* dir_node, struct vnode** target,
                 const char* component_name) {

  printf("[device_lookup] %s\n", component_name);
  struct device_fentry* fentry = (struct device_fentry*)dir_node->internal;
  if (fentry->type != DEV_ROOT) return 0;

  if (!strcmp(component_name, ".")) {
    //printf("[device_lookup] .\n");
    *target = fentry->vnode;
    return 1;
  }
  if (!strcmp(component_name, "..")) {
    //printf("[device_lookup] ..\n");
    if (!fentry->parent_vnode) return 0;
    *target = fentry->parent_vnode;
    return 1;
  }

  for (int i = 0; i < MAX_DEVICE_IN_DIR; i++) {
    fentry = ((struct device_fentry*)dir_node->internal)->child[i];
    printf("[device_lookup] %s\n", fentry->name);
    if (!strcmp(fentry->name, component_name)) {
      *target = fentry->vnode;
      return 1;
    }
  }
  return 0;
}

int _device_create(struct vnode* dir_node,
                 const char* component_name, DEV_TYPE type) {

  for (int i = 0; i < MAX_DEVICE_IN_DIR; i++) {
    struct device_fentry* fentry =
        ((struct device_fentry*)dir_node->internal)->child[i];
    if (fentry->type == DEV_NONE) {
      
      struct vnode* vnode = (struct vnode*)malloc(sizeof(struct vnode));
      vnode->mount = 0;
      if(type == DEV_UART){
        vnode->v_ops = uartfs_v_ops;
        vnode->f_ops = uartfs_f_ops;
      } else {
        printf("[_device_create] error, no such device type\n");
      }
      
      vnode->internal = fentry;
      device_set_fentry(fentry, component_name, vnode, type);
      return 1;
    }
  }
  return -1;
}

void device_list(struct vnode* dir_node) {
  printf("[device_list]: listing dir: %s\n", ((struct device_fentry*)dir_node->internal)->name);
  for (int i = 0; i < MAX_DEVICE_IN_DIR; i++) {
    struct device_fentry* fentry =
        ((struct device_fentry*)dir_node->internal)->child[i];
    if (fentry->type != DEV_NONE) {
      printf("[type]: %d, [name]: %s\n", fentry->type, fentry->name);
    }
  }
  printf("\n");
}

int device_set_parent(struct vnode* child_node, struct vnode* parent_vnode) {
  struct device_fentry* fentry = (struct device_fentry*)child_node->internal;
  fentry->parent_vnode = parent_vnode;
  return 1;
}

int device_create(struct vnode* dir_node, struct vnode** target,
                 const char* component_name, FILE_TYPE type) {
  printf("[device_create] no create function for /dev\n");
  return -1;
}

int device_write(struct file* file, const void* buf, size_t len) {
  /* todo */
  printf("[device_write] no write function for /dev\n");
  return -1;
}

int device_read(struct file* file, void* buf, size_t len) {
  printf("[device_read] no read function for /dev\n");
  return -1;
}



// uart read write functions

int uartfs_read(struct file* file, void* buf, size_t len) {
  size_t read_len = 0;
  struct device_fentry* fentry = (struct device_fentry*)file->vnode->internal;
  for (size_t i = 0; i < len; i++) {
    if(fentry->type == DEV_UART){
      ((char*)buf)[i] = uart_getc();
    }
    read_len++;
  }
  return read_len;
}

int uartfs_write(struct file* file, const void* buf, size_t len) {
  for (size_t i = 0; i < len; i++) {
    uart_send( ((char*)buf)[i] );
  }
  return len;
}

