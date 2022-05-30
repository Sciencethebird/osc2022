#include "mmu.h"

#include <stdint.h>

#include "alloc.h"
#include "mmu_values.h"
#include "printf.h"
#include "thread.h"

void init_page_table(thread_info *thread, uint64_t **table) {
  /*
    uint64_t thread_allocate_page(thread_info *thread, uint64_t size) {
        page_frame *page_frame = buddy_allocate(size);
        thread->page_frame_ids[thread->page_frame_count++] = page_frame->id;
        return page_frame->addr;
    }
  */
  *table = (uint64_t *)thread_allocate_page(thread, PAGE_SIZE);
  for (int i = 0; i < 512; i++) {
    *((*table) + i) = 0;
  }
  // printf("[init] next table virtual addr: 0x%llx\n", (uint64_t)(*table));
  /*
    WHY VA2PA?
        you need to convert VA to PA first since in page table, every entry uses physical address.
        since when you use MMU to transfer address automatically, MMU only walks the table by physical addr, 
        there's no extra entry address translation happen during the process.

        so if you store VA into your table entry, the MMU process will find physical 0xffff..., 
        and there's nothing in the physical 0xffff.....
  */

  *table = (uint64_t *)VA2PA(*table);
}

void update_page_table(thread_info *thread, uint64_t virtual_addr,
                       uint64_t physical_addr, int permission) {
  if (thread->pgd == 0) {
    printf("Invalid PGD!!\n");
    return;
  }

  uint32_t index[4] = {
      (virtual_addr >> 39) & 0x1ff, (virtual_addr >> 30) & 0x1ff,
      (virtual_addr >> 21) & 0x1ff, (virtual_addr >> 12) & 0x1ff};

  // printf("virtual addr: 0x%llx", virtual_addr);
  // printf(", index: 0x%llx", index[0]);
  // printf(", index: 0x%llx", index[1]);
  // printf(", index: 0x%llx", index[2]);
  // printf(", index: 0x%llx\n", index[3]);
  // printf("physical addr: 0x%llx\n", physical_addr);
  
  /*
    In kernel, due to identity mapping, both 0xffff...[addr] and 0x0000...[addr] will be map to the same physical address [addr],
    so it's basically the same thing using 0xffff and 0x0000, why do you need PA2VA?
        - this is because the lower level pgd is already switch to the user pgd (see function switch_pgd() ),
          so if you're still using 0x0000, the MMU will use user pgd to translate your address, 
          to use the kernel space translation, you need to first translate your address to 0xffff so you can use kernel space translation,
          which uses ttbr1_el1 (higher adress table)
  */
  uint64_t *table = (uint64_t *)PA2VA(thread->pgd);
  // printf("table: 0x%llx\n", (uint64_t)table);
  for (int level = 0; level < 3; level++) {
    if (table[index[level]] == 0) {
      //printf("level: %d, index: 0x%llx  ", level, index[level]);
      init_page_table(thread, (uint64_t **)&(table[index[level]]));
      table[index[level]] |= PD_TABLE;
    }
    //printf("table PA: 0x%llx\n", (uint64_t)table[index[level]]);

    /*
        & ~0xfff means & 11111....000
        since, remeber, the least 12 bits of a table entry is used for memory attribute, 
        you need to first filter them out so it gets the actual address of the next table.
    */
    table = (uint64_t *)PA2VA(table[index[level]] & ~0xfff); // filter out memory attribute to get the actual address
    //printf("table VA: 0x%llx\n\n", (uint64_t)table);
  }
  uint64_t BOOT_RWX_ATTR = (1 << 6);
  if (permission & 0b010)
    BOOT_RWX_ATTR |= 1;
  else
    BOOT_RWX_ATTR |= (1 << 7);
  //printf("0x%llx\n", BOOT_RWX_ATTR);
  table[index[3]] = physical_addr | BOOT_PTE_NORMAL_NOCACHE_ATTR | BOOT_RWX_ATTR;
  //printf("page PA: 0x%llx\n", (uint64_t)table[index[3]]);
}


uint64_t user_VA2PA(thread_info *thread, uint64_t user_virtual_addr){
  if (thread->pgd == 0) {
    printf("Invalid PGD!!\n");
    return;
  }

  uint32_t index[4] = {
      (user_virtual_addr >> 39) & 0x1ff, (user_virtual_addr >> 30) & 0x1ff,
      (user_virtual_addr >> 21) & 0x1ff, (user_virtual_addr >> 12) & 0x1ff};

  //printf("virtual addr: 0x%llx", user_virtual_addr);
  //printf(", index: 0x%llx", index[0]);
  //printf(", index: 0x%llx", index[1]);
  //printf(", index: 0x%llx", index[2]);
  //printf(", index: 0x%llx\n", index[3]);
  //printf("physical addr: 0x%llx\n", user_physical_addr);

  uint64_t *table = (uint64_t *)PA2VA(thread->pgd);

  for (int level = 0; level < 3; level++) {
    if (table[index[level]] == 0) {
      printf("[user VA translation error]: %d-th table DNE\n", level);
    }
    table = (uint64_t *)PA2VA(table[index[level]] & ~0xfff); // filter out memory attribute to get the actual address
    //printf("table VA: 0x%llx\n\n", (uint64_t)table);
  }
  printf("page PA: 0x%llx\n", (uint64_t)table[index[3]]);
  uint64_t page_offset = user_virtual_addr & 0xfff;
  return (table[index[3]]& ~0xfff) | page_offset;
}