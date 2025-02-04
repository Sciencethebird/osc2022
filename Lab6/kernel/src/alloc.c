#include "alloc.h"
#include "io.h"

void buddy_test() {
  print_frame_lists();
  uint64_t size[6] = {
      PAGE_SIZE * 1, PAGE_SIZE * 13, PAGE_SIZE * 16,
      PAGE_SIZE * 2, PAGE_SIZE * 4,  PAGE_SIZE * 8,
  };
  page_frame *frame_ptr[6];

  print_s("********** buddy allocation test **********\n");
  for (int i = 0; i < 6; i++) {
    print_s("Press any key to continue...");
    char c = read_c();
    if (c != '\n') print_s("\n");
    frame_ptr[i] = buddy_allocate(size[i]);
    print_s("Successfully allocate ");
    print_i(size[i] / PAGE_SIZE);
    print_s(" pages\n");
    if (c == 'p') print_frame_lists();
  }
  print_s("********** buddy free test **********\n");
  for (int i = 0; i < 6; i++) {
    print_s("Press any key to continue...");
    char c = read_c();
    if (c != '\n') print_s("\n");
    buddy_free(frame_ptr[i]);
    print_s("Successfully free ");
    print_i(size[i] / PAGE_SIZE);
    print_s(" pages\n");
    if (c == 'p') print_frame_lists();
  }
}

void dma_test() {
  print_frame_lists();
  print_dma_list();

  uint64_t size[6] = {
      sizeof(int) * 1, sizeof(int) * 8,    sizeof(int) * 2201,
      sizeof(int) * 9, sizeof(int) * 3068, sizeof(int) * 100,
  };
  void *ptr[6];

  print_s("********** malloc test **********\n");
  for (int i = 0; i < 6; i++) {
    print_s("Press any key to continue...");
    char c = read_c();
    if (c != '\n') print_s("\n");
    ptr[i] = malloc(size[i]);
    print_s("Successfully allocate ");
    print_i(size[i]);
    print_s(" bytes in address ");
    print_h((uint64_t)ptr[i]);
    print_s("\n");
    if (c == 'p') {
      print_frame_lists();
      print_dma_list();
    }
  }
  print_s("********** free test **********\n");
  for (int i = 0; i < 6; i++) {
    print_s("Press any key to continue...");
    char c = read_c();
    if (c != '\n') print_s("\n");
    free(ptr[i]);
    print_s("Successfully free ");
    print_i(size[i]);
    print_s(" bytes in address ");
    print_h((uint64_t)ptr[i]);
    print_s("\n");
    if (c == 'p') {
      print_frame_lists();
      print_dma_list();
    }
  }
}

void buddy_init() {
    // init each page
    for (int i = 0; i < MAX_PAGE_NUM; i++) {
      frames[i].id = i;
      frames[i].order = -1;
      frames[i].is_allocated = 0;
      frames[i].addr = PAGE_BASE_ADDR + i * PAGE_SIZE;
      frames[i].next = 0;
    }
    // no free or used frame in the beggining
    for (int i = 0; i < FRAME_LIST_NUM; i++) {
      free_frame_lists[i] = 0;
      used_frame_lists[i] = 0;
    }
    // assign first frame as the largest single memory frame.
    frames[0].order = MAX_FRAME_ORDER;
    free_frame_lists[MAX_FRAME_ORDER] = &frames[0];
    free_dma_list = 0;
    heap_offset = 0; // for simple_malloc
}

page_frame *buddy_allocate(uint64_t size) {
  uint64_t page_num = size / PAGE_SIZE; // number of 4kb page needed to create the frame.
  if (size % PAGE_SIZE != 0) page_num++; 
  page_num = align_up_exp(page_num);
  uint64_t order = log2(page_num);
  //print_s("allocation size: ");
  //print_i(size);
  //print_s("\r\norder: ");
  //print_i(order);
  //print_s("\r\n");
  for (uint64_t i = order; i <= MAX_FRAME_ORDER; i++) {
    if (free_frame_lists[i]) {
      
      int cur_id = free_frame_lists[i]->id;
      free_frame_lists[i] = free_frame_lists[i]->next;
      frames[cur_id].order = order;
      frames[cur_id].is_allocated = 1;
      frames[cur_id].next = used_frame_lists[order];
      used_frame_lists[order] = &frames[cur_id];
      //print_s("allocate frame index ");
      //print_i(cur_id);
      //print_s(" (4K x 2^");
      //print_i(order);
      //print_s(" = ");
      //print_i(1 << (order + 2));
      //print_s(" KB)\n");

      // release redundant memory block
      for (; i > order; i--) {
        int id = cur_id + (1 << (i - 1));
        frames[id].order = i - 1;
        frames[id].is_allocated = 0;
        frames[id].next = free_frame_lists[i - 1];
        free_frame_lists[i - 1] = &frames[id];
        //print_s("put frame index ");
        //print_i(id);
        //print_s(" back to free lists (4K x 2^");
        //print_i(frames[id].order);
        //print_s(" = ");
        //print_i(1 << (frames[id].order + 2));
        //print_s(" KB)\n");
      }
      // print_s("\n");
      return &frames[cur_id];
    }
  }
  return 0;
}

void buddy_free(page_frame *frame) {
  uint64_t index = frame->id;
  if (!frames[index].is_allocated) {
    print_s("Error: it is already free\n");
    return;
  }

  uint64_t order = frames[index].order;
  buddy_unlink(index, 1); // remove from used list since it's freed
  while (order <= MAX_FRAME_ORDER) {
    // check left right
    uint64_t target_index = index ^ (1 << order);
    // check if target_index is merge-ble.
    if ((target_index >= MAX_PAGE_NUM) || frames[target_index].is_allocated ||
        (frames[target_index].order != order))
      break;

    //print_s("merge with frame index ");
    //print_i(target_index);
    //print_s(" (4K x 2^");
    //print_i(frames[target_index].order);
    //print_s(" = ");
    //print_i(1 << (frames[target_index].order + 2));
    //print_s(" KB)\n");
    buddy_unlink(target_index, 0); // remove from free list since it's merged
    order += 1;
    // the index need to point to the head of that memory frame.
    if (index > target_index) index = target_index;
  }
  frames[index].order = order;
  frames[index].next = free_frame_lists[order];
  free_frame_lists[order] = &frames[index];
  // print_s("put frame index ");
  // print_i(index);
  // print_s(" back (4K x 2^");
  // print_i(frames[index].order);
  // print_s(" = ");
  // print_i(1 << (frames[index].order + 2));
  // print_s(" KB)\n\n");
}

void buddy_unlink(int index, int type) {
  uint64_t order = frames[index].order;
  print_s(""); // I don't know why you need this print
  frames[index].order = -1;
  print_s(""); // I don't know why you need this print
  frames[index].is_allocated = 0;

  if (type == 0) {
    if (free_frame_lists[order] == &frames[index]) {
      // move out of free frame list since it's been merged.
      free_frame_lists[order] = frames[index].next;
      frames[index].next = 0;
    } else {
      for (page_frame *cur = free_frame_lists[order]; cur; cur = cur->next) {
        if (cur->next == &frames[index]) {
          cur->next = frames[index].next;
          frames[index].next = 0;
          break;
        }
      }
    }
  }
  if (type == 1) {
    if (used_frame_lists[order] == &frames[index]) {
      used_frame_lists[order] = frames[index].next;
      frames[index].next = 0;
    } else {
      for (page_frame *cur = used_frame_lists[order]; cur; cur = cur->next) {
        if (cur->next == &frames[index]) {
          cur->next = frames[index].next;
          frames[index].next = 0;
          break;
        }
      }
    }
  }
}

void print_frame_lists() {
  print_s("========================\n");
  print_s("Free frame lists: \n");
  for (int i = MAX_FRAME_ORDER; i >= 0; i--) {
    print_s("4K x 2^");
    print_i(i);
    print_s(" (");
    print_i(1 << (i + 2));
    print_s(" KB):");
    for (page_frame *cur = free_frame_lists[i]; cur; cur = cur->next) {
      print_s("  index ");
      print_i(cur->id);
      print_s("(");
      print_h(cur->addr);
      print_s(")");
    }
    print_s("\n");
  }
  print_s("========================\n");
}

void *malloc(uint64_t size) {
  dma_header *free_slot = 0;
  uint64_t min_size = ((uint64_t)1) << 63;
  // find the smallest free slot which is bigger than the required size
  for (dma_header *cur = free_dma_list; cur; cur = cur->next) {
    // minus dma_header size, allocatable size.
    uint64_t data_size = cur->total_size - align_up(sizeof(dma_header), 8);
    if (data_size >= align_up(size, 8) && data_size < min_size) {
      free_slot = cur;
      min_size = data_size;
    }
  }

  uint64_t allocated_size = align_up(sizeof(dma_header), 8) + align_up(size, 8);
  if (free_slot) {
    uint64_t addr = (uint64_t)free_slot;
    uint64_t total_size = free_slot->total_size;

    // rewrite the found free slot
    free_slot->total_size = allocated_size;
    free_slot->used_size = size;
    free_slot->is_allocated = 1;
    // cut from free_slot linked-list
    if (free_slot->prev) free_slot->prev->next = free_slot->next;
    if (free_slot->next) free_slot->next->prev = free_slot->prev;
    if (free_dma_list == free_slot) free_dma_list = free_slot->next;
    free_slot->prev = 0;
    free_slot->next = 0;

    // create another free slot if remaining size is big enough
    int64_t free_size =
        total_size - allocated_size - align_up(sizeof(dma_header), 8);
    if (free_size > 0) {
      dma_header *new_header = (dma_header *)(addr + allocated_size);
      new_header->total_size = total_size - allocated_size;
      new_header->used_size = 0;
      new_header->is_allocated = 0;
      new_header->frame_ptr = free_slot->frame_ptr;
      new_header->prev = 0;
      new_header->next = free_dma_list;
      if (free_dma_list) free_dma_list->prev = new_header;
      free_dma_list = new_header;
    } else {
      free_slot->total_size = total_size;
    }
    return (void *)(addr + align_up(sizeof(dma_header), 8));
  } else {
    // allocate a page
    page_frame *frame_ptr = buddy_allocate(allocated_size);
    uint64_t addr = frame_ptr->addr;
    // create a free slot
    dma_header *allocated_header = (dma_header *)addr;
    allocated_header->total_size = allocated_size;
    allocated_header->used_size = size;
    allocated_header->is_allocated = 1;
    allocated_header->frame_ptr = frame_ptr;
    allocated_header->prev = 0;
    allocated_header->next = 0;

    // create another free slot if remaining size is big enough
    uint64_t order = frame_ptr->order;
    uint64_t total_size = (1 << order) * 4 * kb;
    int64_t free_size = total_size - allocated_size - align_up(sizeof(dma_header), 8);
    if (free_size > 0) {
      dma_header *new_header = (dma_header *)(addr + allocated_size);
      new_header->total_size = total_size - allocated_size;
      new_header->used_size = 0;
      new_header->is_allocated = 0;
      new_header->frame_ptr = frame_ptr;
      new_header->prev = 0;
      new_header->next = free_dma_list;
      if (free_dma_list) free_dma_list->prev = new_header;
      free_dma_list = new_header;
    } else {
      allocated_header->total_size = total_size;
    }
    return (void *)(addr + align_up(sizeof(dma_header), 8));
  }
  return 0;
}

void free(void *ptr) {
  uint64_t target_addr = (uint64_t)ptr - align_up(sizeof(dma_header), 8);
  dma_header *target_header = (dma_header *)target_addr;
  target_header->used_size = 0;
  target_header->is_allocated = 0;
  target_header->prev = 0;
  target_header->next = free_dma_list;
  if (free_dma_list) free_dma_list->prev = target_header;
  free_dma_list = target_header;
  
  //enters the frame and merge dma in the frame if possible
  page_frame *frame_ptr = target_header->frame_ptr;
  uint64_t base_addr = frame_ptr->addr;
  uint64_t order = frame_ptr->order;
  uint64_t total_frame_size = (1 << order) * 4 * kb;
  uint64_t boundary = base_addr + total_frame_size;

  // merge next slot if it is free
  uint64_t next_addr = target_addr + target_header->total_size;
  dma_header *next_header = (dma_header *)next_addr;
  if (next_addr < boundary && !next_header->is_allocated) {
    if (next_header->prev) next_header->prev->next = next_header->next;
    if (next_header->next) next_header->next->prev = next_header->prev;
    if (free_dma_list == next_header) free_dma_list = next_header->next;
    next_header->prev = 0;
    next_header->next = 0;
    target_header->total_size += next_header->total_size;
  }

  // merge previous slot if it is free, check from start
  uint64_t current_addr = base_addr;
  while (current_addr < boundary) {
    dma_header *header = (dma_header *)current_addr;
    uint64_t next_addr = current_addr + header->total_size;
    if (next_addr == target_addr) {
      if (!header->is_allocated) {
        header->total_size += target_header->total_size;
        if (target_header->prev)
          target_header->prev->next = target_header->next;
        if (target_header->next)
          target_header->next->prev = target_header->prev;
        if (free_dma_list == target_header) free_dma_list = target_header->next;
        target_header->prev = 0;
        target_header->next = 0;
      }
      break;
    }
    current_addr = next_addr;
  }

  // free page frame if all slots are free
  dma_header *base_header = (dma_header *)base_addr;
  if (base_header->total_size == total_frame_size) {
    if (base_header->prev) base_header->prev->next = base_header->next;
    if (base_header->next) base_header->next->prev = base_header->prev;
    if (free_dma_list == base_header) free_dma_list = base_header->next;
    base_header->prev = 0;
    base_header->next = 0;
    buddy_free(frame_ptr);
  }
}

void print_dma_list() {
  print_s("========================\n");
  print_s("Free DMA slots: \n");
  for (dma_header *cur = free_dma_list; cur; cur = cur->next) {
    print_s("size: ");
    print_i(cur->total_size - align_up(sizeof(dma_header), 8));
    print_s(", frame index: ");
    print_i(cur->frame_ptr->id);
    print_s("\n");
  }
  print_s("========================\n");
}


void* simple_malloc(size_t size){
    size_t mem_start = heap_offset + HEAP_START;
    heap_offset+=size;
    return (void*) mem_start;
}

unsigned long __stack_chk_guard;
void __stack_chk_guard_setup(void)
{
     __stack_chk_guard = 0xBAAAAAAD;//provide some magic numbers
}

void __stack_chk_fail(void)                         
{
 /* Error message */                                 
}// will be called when guard variable is corrupted 


void memcpy(void *dest, const void *src, unsigned int n)
{
    for (unsigned int i = 0; i < n; i++)
    {
        ((char*)dest)[i] = ((char*)src)[i];
    }
}
