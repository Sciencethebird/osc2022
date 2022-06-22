#include "fatfs.h"

#include "alloc.h"
#include "printf.h"
#include "sdhost.h"
#include "string.h"
#include "vfs.h"

int get_starting_sector(int cluster) {
  return (cluster - 2) * fat_boot_sector->sectors_per_cluster +
         data_starting_sector;
}

void fatfs_init() {
  sd_init();
  fatfs_v_ops =
      (struct vnode_operations*)malloc(sizeof(struct vnode_operations));
  fatfs_v_ops->lookup = fatfs_lookup;
  fatfs_v_ops->set_parent = fatfs_set_parent;
  fatfs_v_ops->create = fatfs_create;
  fatfs_f_ops = (struct file_operations*)malloc(sizeof(struct file_operations));
  fatfs_f_ops->write = fatfs_write;
  fatfs_f_ops->read = fatfs_read;
  fatfs_f_ops->list = fatfs_list;
}

void fatfs_set_directory(struct fatfs_fentry* fentry,
                         struct fatfs_dentry* dentry) {
  for (int i = 0; i < MAX_FILES_IN_DIR; ++i) {
    int flag = 0;
    for (int j = 0; j < 8; j++) {
      // printf("0x%x ", (dentry + i)->filename[j]);
      // printf("0x%x ", (dentry + i)->filename[j]);
      //printf("%3d", (dentry + i)->filename[j]);
      // handle weird file
      if ((dentry + i)->filename[j] == 0) {
        flag = 1;
      }
    }
    
    //printf("\n\n[fatfs_set_directory] file name: %s\n", (dentry + i)->filename);
    //printf("flag: %d\n", flag);
    //printf("[fatfs_set_directory]dentry + i=%d, %d\n", dentry + i, i);

    if ((dentry + i)->filename[0] && !flag) {
      
      fatfs_dentry_template = *(dentry + i);
      strncpy((char*)fentry->child[i]->name, (char*)(dentry + i)->filename, 8);
      size_t len = strlen((char*)fentry->child[i]->name);
      fentry->child[i]->name_len = len;
      if ((dentry + i)->extension[0]) {
        *(fentry->child[i]->name + len) = '.';
        strncpy((char*)fentry->child[i]->name + len + 1, (char*)(dentry + i)->extension, 3);
      }

      struct vnode* vnode = (struct vnode*)malloc(sizeof(struct vnode));
      vnode->mount = 0;
      vnode->v_ops = fentry->vnode->v_ops;
      vnode->f_ops = fentry->vnode->f_ops;
      vnode->internal = fentry->child[i];
      int starting_cluster =
          ((dentry + i)->cluster_high << 16) + (dentry + i)->cluster_low;
      int buf_size = (dentry + i)->file_size;
      printf("[fatfs_set_directory] file name: %s, starting cluster: %x, %d\n", 
        (char*)fentry->child[i]->name, starting_cluster, starting_cluster);
      //printf("cluster high: %d\n", (dentry + i)->cluster_high);
      //printf("starting cluster: %d, real:%d, size:%d\n\n", 
      //      starting_cluster, get_starting_sector(starting_cluster), (dentry + i)->file_size );
      char* temp= (char*)malloc(BLOCK_SIZE);
      readblock(get_starting_sector(starting_cluster), temp);
      //printf("number:\n");
      //for(int i = 0; i< BLOCK_SIZE; i++){
      //  printf("%d", temp[i]);
      //} printf("\n");
      //printf("ascii:\n");
      //for(int i = 0; i< BLOCK_SIZE; i++){
      //  printf("%c", temp[i]);
      //} printf("\n");
      fatfs_set_fentry(fentry->child[i], FILE_REGULAR, vnode, starting_cluster,
                       buf_size);
      // a large file?????
      // is cluster fully used a sector???
      // test: a big file memory print test.
      // printf("%s\n", fentry->child[i]->name);
    }
  }
}

void fatfs_set_fentry(struct fatfs_fentry* fentry, FILE_TYPE type,
                      struct vnode* vnode, int starting_cluster, int buf_size) {
  fentry->starting_cluster = starting_cluster;
  fentry->vnode = vnode;
  fentry->type = type;
  fentry->buf = (struct fatfs_buf*)malloc(sizeof(struct fatfs_buf));
  fentry->buf->size = buf_size;
  for (int i = 0; i < FATFS_BUF_SIZE; i++) {
    fentry->buf->buffer[i] = '\0';
  }

  if (fentry->type == FILE_DIRECTORY) {
    for (int i = 0; i < MAX_FILES_IN_DIR; ++i) {
      fentry->child[i] =
          (struct fatfs_fentry*)malloc(sizeof(struct fatfs_fentry));
      fentry->child[i]->name[0] = 0;
      fentry->child[i]->type = FILE_NONE;
      fentry->child[i]->parent_vnode = vnode;
    }
  }
}

int fatfs_setup_mount(struct filesystem* fs, struct mount* mount) {
  
  // read block 0 to get MBR
  char* mbr = (char*)malloc(BLOCK_SIZE);
  readblock(0, mbr);
  if (mbr[510] != 0x55 || mbr[511] != 0xAA) {
    printf("[fatfs_init] bad magic in MBR\n");
    return 0;
  }

  // in MBR get the first partition entry
  int entry_size = sizeof(struct mbr_partition_entry);
  struct mbr_partition_entry* entry =
      (struct mbr_partition_entry*)malloc(entry_size);
  char* src = (char*)mbr;
  char* dst = (char*)entry;
  for (int i = 0; i < entry_size; i++) {
    // printf("i: %d, 0x%x\n", i, src[MBR_PARTITION_BASE + i]);
    dst[i] = src[MBR_PARTITION_BASE + i];
  }
  free(mbr);

  printf("\n========== FAT32 init ==========\n");
  printf("Partition type: 0x%x", entry->partition_type);
  if (entry->partition_type == 0xB) {
    printf(" (FAT32 with CHS addressing)");
  }
  printf("\nPartition size: %d (sectors)\n", entry->sector_count);
  printf("Block index: %d\n", entry->starting_sector);
  printf("================================\n\n");

  // get the starting sector (Volume ID sector) in the MBR entry
  fat_starting_sector = entry->starting_sector;

  char* fat_boot = (char*)malloc(BLOCK_SIZE);
  readblock(fat_starting_sector, fat_boot);
  int boot_sector_size = sizeof(struct fatfs_boot_sector);
  fat_boot_sector = (struct fatfs_boot_sector*)malloc(boot_sector_size);
  src = (char*)fat_boot;
  dst = (char*)fat_boot_sector;
  for (int i = 0; i < boot_sector_size; i++) {
    dst[i] = src[i];
  }
  free(fat_boot);

  // find the root directory sector
  int root_dir_sectors = 0;  // no root directory sector in FAT32
  // MBR -> volumn ID -> FAT1, FAT2 -> data clusters
  data_starting_sector =
      fat_starting_sector + fat_boot_sector->reserved_sector_count +
      fat_boot_sector->fat_count * fat_boot_sector->sectors_per_fat_32 +
      root_dir_sectors;

  // find fat table sector and table number
  fat_table_starting_sector = fat_starting_sector + fat_boot_sector->reserved_sector_count;
  fat_table_num = fat_boot_sector->fat_count * fat_boot_sector->sectors_per_fat_32;

  for(int n = 0; n<50; n++) {
    print_fat_table(n);
  }
  

  // get the root dir sector, which stores files
  root_starting_sector = get_starting_sector(fat_boot_sector->root_cluster);

  char* fat_root = (char*)malloc(BLOCK_SIZE);
  readblock(root_starting_sector, fat_root);

  // read the root dir sector, which contains dir entries
  fat_root_dentry = (struct fatfs_dentry*)fat_root;

  struct fatfs_fentry* root_fentry =
      (struct fatfs_fentry*)malloc(sizeof(struct fatfs_fentry));
  struct vnode* vnode = (struct vnode*)malloc(sizeof(struct vnode));
  vnode->mount = mount;
  vnode->v_ops = fatfs_v_ops;
  vnode->f_ops = fatfs_f_ops;
  vnode->internal = (void*)root_fentry;
  root_fentry->parent_vnode = 0;

  fatfs_set_fentry(root_fentry, FILE_DIRECTORY, vnode,
                   fat_boot_sector->root_cluster, 4096);
  fatfs_set_directory(root_fentry, fat_root_dentry);

  mount->fs = fs;
  mount->root = vnode;
  return 1;
}

int fatfs_lookup(struct vnode* dir_node, struct vnode** target,
                 const char* component_name) {
  // printf("[lookup] %s\n", component_name);
  struct fatfs_fentry* fentry = (struct fatfs_fentry*)dir_node->internal;
  if (fentry->type != FILE_DIRECTORY) return 0;

  if (!strcmp(component_name, ".")) {
    *target = fentry->vnode;
    return 1;
  }
  if (!strcmp(component_name, "..")) {
    if (!fentry->parent_vnode) return 0;
    *target = fentry->parent_vnode;
    return 1;
  }

  for (int i = 0; i < MAX_FILES_IN_DIR; i++) {
    fentry = ((struct fatfs_fentry*)dir_node->internal)->child[i];
    if (!strcmp(fentry->name, component_name)) {
      *target = fentry->vnode;
      return 1;
    }
  }
  return 0;
}

int fatfs_set_parent(struct vnode* child_node, struct vnode* parent_vnode) {
  struct fatfs_fentry* fentry = (struct fatfs_fentry*)child_node->internal;
  fentry->parent_vnode = parent_vnode;
  return 1;
}

int fatfs_write(struct file* file, const void* buf, size_t len) {
  struct fatfs_fentry* fentry = (struct fatfs_fentry*)file->vnode->internal;
  
  // write data to buffer and update file position 
  for (size_t i = 0; i < len; i++) {
    fentry->buf->buffer[file->f_pos++] = ((char*)buf)[i];
    if (fentry->buf->size < file->f_pos) {
      fentry->buf->size = file->f_pos;
    }
  }

  // write buffer to the sd card
  int data_sector = fentry->starting_cluster;

  //writeblock(starting_sector, fentry->buf->buffer);
  for (size_t i = 0; i < fentry->buf->size; i+=512) {
    writeblock(get_starting_sector(data_sector), fentry->buf->buffer+i);
    int new_entry = find_free_fat_table_entry();
    set_free_fat_table_entry(data_sector, new_entry);
    data_sector = new_entry;
  }
  set_free_fat_table_entry(data_sector, TABLE_TERMINATE);

  // update file position in root directory
  for (int i = 0; i < MAX_FILES_IN_DIR; i++) {
    // init a null string
    char full_name[12];
    for(int j = 0; j < 11; j++){
      full_name[j] = '\0';
    }

    strncpy(full_name, (char*)(fat_root_dentry + i)->filename, 8);
    full_name[strlen(full_name)] = '.';
    strncpy(full_name+strlen(full_name), (char*)(fat_root_dentry + i)->extension, 3);
    
    if (!strcmp(full_name, fentry->name)) {
      (fat_root_dentry + i)->file_size = fentry->buf->size;
      printf("\n[!!!]new file size: %d\n", (fat_root_dentry + i)->file_size);
    }
  }
  // update root dir table in the sd card
  writeblock(root_starting_sector, (char*)fat_root_dentry);

  return len;
}

int fatfs_read(struct file* file, void* buf, size_t len) {
  size_t read_len = 0;
  struct fatfs_fentry* fentry = (struct fatfs_fentry*)file->vnode->internal;
  int data_sector = fentry->starting_cluster;

  printf("[buffer size] size: %d", fentry->buf->size);
  for(int i = 0; ; i++) {
    readblock( get_starting_sector(data_sector), fentry->buf->buffer + i*512);
    printf("current sector: %x", data_sector);
    data_sector = get_fat_table_entry(data_sector);
    printf(" ,next sector: %x\n", data_sector);
    if(data_sector == TABLE_TERMINATE) break;
  }
  
  //readblock(starting_sector, fentry->buf->buffer);
  printf("[read pos]: %d\n", file->f_pos);
  for (size_t i = 0; i < len; i++) {
    ((char*)buf)[i] = fentry->buf->buffer[file->f_pos++];
    read_len++;
    if (read_len == fentry->buf->size) {
      break;
    }
  }
  return read_len;
}

void fatfs_list(struct vnode* dir_node) {
  printf("[fatfs_list]: listing dir: %s\n", ((struct fatfs_fentry*)dir_node->internal)->name);
  for (int i = 0; i < MAX_FILES_IN_DIR; i++) {
    struct fatfs_fentry* fentry =
        ((struct fatfs_fentry*)dir_node->internal)->child[i];
    if (fentry->type != FILE_NONE) {
      printf(" [type]: %d, [name]: %s\n", fentry->type, fentry->name);
    }
  }
  printf("\n");
}

int fatfs_create(struct vnode* dir_node, struct vnode** target,
                 const char* component_name, FILE_TYPE type) {

    printf("[fatfs_create] called!\n");
    struct fatfs_dentry* dentry = fat_root_dentry;
    for (int i = 0; i < MAX_FILES_IN_DIR; ++i) {
      int flag = 0;
      printf("[fatfs_create] i: %d, filename: %s\n", i, (dentry + i)->filename);
      for (int j = 0; j < 8; j++) {
        // printf("0x%x ", (dentry + i)->filename[j]);
        // printf("0x%x ", (dentry + i)->filename[j]);
        // printf("%3d", (dentry + i)->filename[j]);
        // handle weird file
        if ((dentry + i)->filename[j] == 0 ) {
          flag = 1;
        }
      }
      if (flag && i != 0) {

        printf("[fatfs_create] flag: %d\n", flag);
        struct fatfs_fentry* fentry;
        for (int i = 0; i < MAX_FILES_IN_DIR; i++) {
          struct fatfs_fentry* temp =
              ((struct fatfs_fentry*)dir_node->internal)->child[i];
          if (temp->type == FILE_NONE) {
            fentry = temp;
            break;
          }
        }
        printf("component_name: %s\n", component_name);
        strcpy(fentry->name, component_name);
        size_t len = strlen(fentry->name);
        fentry->name_len = len;       
        
        struct vnode* vnode = (struct vnode*)malloc(sizeof(struct vnode));
        vnode->mount = 0;
        vnode->v_ops = fatfs_v_ops;
        vnode->f_ops = fatfs_f_ops;
        vnode->internal = fentry;

        //printf("[fatfs_create] %s\n", strtok(component_name, '.'));
        *(dentry + i) = fatfs_dentry_template;
        for(int k = 0; k< 11; k++){
          (dentry + i)->filename[k] = ' ';
        }
        char filename[20];
        strcpy(filename, component_name);
        strtok(filename, '.');
        char* extension = (char*)component_name + strlen(filename)+1;
        for(int k = 0; k< strlen(filename); k++){
          (dentry + i)->filename[k] = filename[k];
        }
        strncpy((char*)(dentry + i)->extension, extension, 3);
        printf("[fatfs_create] name: %s.\n", (char*)(dentry + i)->filename);
        printf("[fatfs_create] ext: %s\n\n", (char*)(dentry + i)->extension);

        // find a new entry on the fat table
        int new_table_entry = find_free_fat_table_entry();
        printf("\n\n new table entry: %d", new_table_entry);
        (dentry + i)->cluster_high = new_table_entry >> (16);
        (dentry + i)->cluster_low = new_table_entry & 0xFFFF;
        set_free_fat_table_entry(new_table_entry, 0); // when init size = 0

        printf("\n\n\n[create] %s\n", (char*)(fat_root_dentry + i)->filename);
        // update root directory table with new directory table
        writeblock(root_starting_sector, (char*)fat_root_dentry);
        
        // data cluster number == location of new_table_entry
        fatfs_set_fentry(fentry, FILE_REGULAR, vnode, new_table_entry, 1);

        *target = fentry->vnode;

        printf("root table update done!!!!");
        return 1;
      }
    }
  return -1;                  
}

void print_fat_table(int table_index) {
  char fat_table[512];
  readblock(table_index + fat_table_starting_sector, fat_table);
  printf("\n===================================================================== table sector: %d ===================================================================\n", table_index);
  for(int i = 0; i< 128; i++) {
    if(i%8 == 0) printf("| ");
    printf("[%4x] = %8x |", i+table_index*128, ((int*)fat_table)[i]);
     if(i%8 == 7) printf("\n");
  }
  printf("==========================================================================================================================================================\n", table_index);
}

int find_free_fat_table_entry() {
  char fat_table[512];
  for(int i = 0; i < fat_table_num; i++) {
    readblock(i + fat_table_starting_sector, fat_table);
    for(int j = 0; j< 128; j++) {
      if( ((unsigned int*)fat_table)[j] == 0 ) {
        printf("[find_free_fat_table_entry] find free entry at %x\n", j+128*i);
        return  j+128*i;
      }
    }
  }
}

int set_free_fat_table_entry(int entry_index, int value) {
  int sector_index = entry_index / 128;
  int in_sector_index = entry_index % 128;
  char fat_table[512];
  readblock(sector_index + fat_table_starting_sector, fat_table);
  print_fat_table(sector_index);
  ((int*)fat_table)[in_sector_index] = value;
  writeblock(sector_index + fat_table_starting_sector, fat_table);
  print_fat_table(sector_index);
}

int get_fat_table_entry(int entry_index) {
  char fat_table[512];
  int sector_index = entry_index / 128;
  int in_sector_index = entry_index % 128;
  readblock(sector_index + fat_table_starting_sector, fat_table);
  return ((int*)fat_table)[in_sector_index];
}