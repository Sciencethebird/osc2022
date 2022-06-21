
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

void fbfs_init() {
  fbfs_v_ops =
      (struct vnode_operations*)malloc(sizeof(struct vnode_operations));
  fbfs_v_ops->lookup = device_lookup;
  fbfs_v_ops->create = device_create;
  fbfs_v_ops->set_parent = device_set_parent;
  fbfs_f_ops = (struct file_operations*)malloc(sizeof(struct file_operations));
  fbfs_f_ops->write = fbfs_write;
  fbfs_f_ops->read = fbfs_read;
  fbfs_f_ops->ioctl = fbfs_ioctl;
  fbfs_f_ops->list = device_list;
}

void device_init() {
  root_init();
  uartfs_init();
  fbfs_init();
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
  device_set_fentry(root_fentry, "/", vnode, DEV_ROOT);
  mount->fs = fs;
  mount->root = vnode;
  
  // create device vnode
  _device_create(mount->root, "uart", DEV_UART);
  _device_create(mount->root, "framebuffer", DEV_FB);
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
      } else if (type == DEV_FB){
        vnode->v_ops = fbfs_v_ops;
        vnode->f_ops = fbfs_f_ops;
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

int fbfs_read(struct file* file, void* buf, size_t len) {
  printf("[fbfs_read] not implemented\n");
  return -1;
}

int fbfs_write(struct file* file, const void* buf, size_t len) {
  //printf("[fbfs_write] len %d\n", len);
  //printf("%d, %d, %d, %d\n", ((char*)buf)[0], ((char*)buf)[1], ((char*)buf)[2], ((char*)buf)[3]);
  if(len != 4){
    printf("[fbfs_write] write len error: %d\n", len);
    return -1;
  }
  
  if(isrgb==1){
    lfb[file->f_pos++] = ((char*)buf)[0];
    lfb[file->f_pos++] = ((char*)buf)[1];
    lfb[file->f_pos++] = ((char*)buf)[2];
    lfb[file->f_pos++] = ((char*)buf)[3];
  } else {
    lfb[file->f_pos++] = ((char*)buf)[2];
    lfb[file->f_pos++] = ((char*)buf)[1];
    lfb[file->f_pos++] = ((char*)buf)[0];
    lfb[file->f_pos++] = ((char*)buf)[3];
  }
  return 4;
}

int fbfs_ioctl(struct file* file, unsigned long request, void* args) {
  struct framebuffer_info* info = (struct framebuffer_info*) args;
  printf("[fbfs_ioctl] framebuffer_info: w: %d, h: %d, pitch: %d, is_rgb: %d\n", 
        info->width, info->height, info->pitch, info->isrgb);
  if(request == 0) {
    mbox[0] = 35 * 4;
    mbox[1] = MBOX_REQUEST;

    mbox[2] = 0x48003; // set phy wh
    mbox[3] = 8;
    mbox[4] = 8;
    mbox[5] = info->width; // FrameBufferInfo.width
    mbox[6] = info->height;  // FrameBufferInfo.height

    mbox[7] = 0x48004; // set virt wh
    mbox[8] = 8;
    mbox[9] = 8;
    mbox[10] = 1024; // FrameBufferInfo.virtual_width
    mbox[11] = 768;  // FrameBufferInfo.virtual_height

    mbox[12] = 0x48009; // set virt offset
    mbox[13] = 8;
    mbox[14] = 8;
    mbox[15] = 0; // FrameBufferInfo.x_offset
    mbox[16] = 0; // FrameBufferInfo.y.offset

    mbox[17] = 0x48005; // set depth
    mbox[18] = 4;
    mbox[19] = 4;
    mbox[20] = 32; // FrameBufferInfo.depth

    mbox[21] = 0x48006; // set pixel order
    mbox[22] = 4;
    mbox[23] = 4;
    mbox[24] = info->isrgb; // RGB, not BGR preferably

    mbox[25] = 0x40001; // get framebuffer, gets alignment on request
    mbox[26] = 8;
    mbox[27] = 8;
    mbox[28] = 4096; // FrameBufferInfo.pointer
    mbox[29] = 0;    // FrameBufferInfo.size

    mbox[30] = 0x40008; // get pitch
    mbox[31] = 4;
    mbox[32] = 4;
    mbox[33] = info->pitch; // FrameBufferInfo.pitch

    mbox[34] = MBOX_TAG_LAST;
    
    // this might not return exactly what we asked for, could be
    // the closest supported resolution instead
    if (mbox_call(MBOX_CH_PROP) && mbox[20] == 32 && mbox[28] != 0) {
      mbox[28] &= 0x3FFFFFFF; // convert GPU address to ARM address
      width = mbox[5];        // get actual physical width
      height = mbox[6];       // get actual physical height
      pitch = mbox[33];       // get number of bytes per line
      isrgb = mbox[24];       // get the actual channel order
      lfb = (void *)((unsigned long)mbox[28]);
      printf("[fbfs_ioctl] set screen resolution: %dx%d, %d, %d, at: %x\n",
             width, height, pitch, isrgb, lfb);
    } else {
      printf("Unable to set screen resolution to 1024x768x32\n");
    }
    return 0;
  } else {
    printf("[ioctl] request not implemented.\n");
    return -1;
  }
}
