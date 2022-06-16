#include "thread.h"
#include "timer.h"
#include "string.h"
#include "io.h"
#include "alloc.h"
#include "printf.h"
#include "utils.h"
#include "kernel.h"
#include "mmu.h"

char* exec_program_name;

void thread_init() {
  run_queue.head = 0;
  run_queue.tail = 0;
  thread_cnt = 0;
}

thread_info *thread_create(void (*func)()) {
  thread_info *thread = (thread_info *)malloc(sizeof(thread_info));

  uint64_t *pgd;
  asm volatile("mrs %0, ttbr1_el1" : "=r"(pgd));
  thread->pgd = pgd;
  for (int i = 0; i < MAX_PAGE_FRAME_PER_THREAD; i++)
    thread->page_frame_ids[i] = 0;
  thread->page_frame_count = 0;

  thread->pid = thread_cnt++;
  thread->status = THREAD_READY;
  thread->next = 0;
  thread->kernel_stack_base = thread_allocate_page(thread, STACK_SIZE);
  thread->user_stack_base = 0;
  thread->user_program_base = 0;
  thread->context.fp = thread->kernel_stack_base + STACK_SIZE;
  thread->context.lr = (uint64_t)func;
  thread->context.sp = thread->kernel_stack_base + STACK_SIZE;
  for (int i = 0; i < FD_MAX; ++i) thread->fd_table.files[i] = 0;

  run_queue_push(thread);
  return thread;
}


void run_queue_push(thread_info *thread) {
  if (run_queue.head == 0) {
    run_queue.head = run_queue.tail = thread;
  } else {
    // printf("%p\n", run_queue.tail);
    // printf("%p\n", run_queue.tail->next);
    run_queue.tail->next = thread;
    run_queue.tail = thread;
  }
}

void schedule() {
  //print_s("scheduling\r\n");
  //printf("run queue head: %d\n",run_queue.head);
  // no other thread to run
  if (run_queue.head == 0) {
    printf("nothing to run\n");
    //plan_next_interrupt_tval(SCHEDULE_TVAL);
    //core_timer_enable(SCHEDULE_TVAL);
    core_timer_disable();
    enable_interrupt();
    run_shell();
    return;
  }
  // check if there's any other thread to run
  if (run_queue.head == run_queue.tail) {  // idle thread
    free(run_queue.head);
    run_queue.head = run_queue.tail = 0;
    thread_cnt = 0;
    return;
  }

  do {
    //print_s("dfdf\r\n");
    run_queue.tail->next = run_queue.head;
    run_queue.tail = run_queue.head;
    run_queue.head = run_queue.head->next;
    run_queue.tail->next = 0;
  } while (run_queue.head->status != THREAD_READY);
  //enable_interrupt();
  //plan_next_interrupt_tval(SCHEDULE_TVAL);
  //printf("current pid: %d\n", run_queue.head->pid);
  plan_next_interrupt_tval(SCHEDULE_TVAL);
  
  switch_pgd((uint64_t)(run_queue.head->pgd));
  enable_interrupt();
  switch_to((uint64_t)get_current(), (uint64_t)run_queue.head);

  //uint64_t sp;
  //asm volatile("mov %0, sp\n" : "=r"(sp) :);
  //printf("pid: %d, sp: %\n", get_current()->pid,sp);
}

void idle() {
  while (1) {
    //printf("idle\n");
    disable_interrupt();
    handle_fork();
    kill_zombies();
    enable_interrupt();
    schedule();
  }
}

void exit() {
  // disable_interrupt();
  // thread_info *cur = current_thread();
  // cur->status = THREAD_DEAD;
  // kill all thread in this lab, formally, you have to maitain a child pid list and kill child too.
  for (thread_info *ptr = run_queue.head; ptr->next != 0; ptr = ptr->next) {
      ptr->status = THREAD_DEAD;
      //thread_free_page(ptr);
  }

  //thread_free_page(cur);
  schedule();
}

void kill(int pid){
  printf("killing child process with pid = %d\n", pid);
  for (thread_info *ptr = run_queue.head; ptr->next != 0; ptr = ptr->next) {
    if(ptr->pid == pid){
      printf("found child");
      ptr->status = THREAD_DEAD;
    }
  }
}

void kill_zombies() {
  //disable_interrupt();
  //printf("killing zombies\n");
  if (run_queue.head == 0) return;
  //print_s("killing zombies 2222\r\n");
  for (thread_info *ptr = run_queue.head; ptr->next != 0; ptr = ptr->next) {
    //print_s("finding zombie\r\n");
    //printf("[pid:%d, status:%d] -> ", ptr->pid, ptr->status);
    for (thread_info *cur = ptr->next; cur != 0 && cur->status == THREAD_DEAD; /**/ ) {
      thread_info *tmp = cur->next;
      // printf("find dead thread %d\n", cur->tid);
      printf("find dead thread, pid: %d\n", cur->pid);
       printf("curr pid: %d\n", get_current()->pid);
      printf("pgd: %x\n", get_current()->pgd);
      free((void *)cur);
      ptr->next = tmp;
      cur = tmp;
    }
    if (ptr->next == 0) {
      run_queue.tail = ptr;
      break;
    }
  }
}

thread_info *current_thread() {
  thread_info *ptr;
  asm volatile("mrs %0, tpidr_el1\n" : "=r"(ptr) :);
  return ptr;
}

void timer_schedular_handler(){
  //kill_zombies();
  //handle_fork();
  schedule();
  //print_s("set next interrupt\r\n");
}

void fork(uint64_t sp) {
  run_queue.head->status = THREAD_FORK;
  run_queue.head->trap_frame_addr = sp;
  schedule();
  trap_frame_t *trap_frame = (trap_frame_t *)(get_current()->trap_frame_addr);
  trap_frame->x[0] = run_queue.head->child_pid;
  printf("fork return: %d\n", trap_frame->x[0]);
}

void handle_fork() {
  for (thread_info *ptr = run_queue.head->next; ptr != 0; ptr = ptr->next) {
    //printf("ptr: %d\n", ptr);
    if ((ptr->status) == THREAD_FORK) {
      //printf("creating child thread\n");
      thread_info *child = thread_create(0);
      //printf("\n\ncreate done\n");
      create_child(ptr, child);
      //printf("\n\ncreate child done\n");
      ptr->status = THREAD_READY;
      child->status = THREAD_READY;
    }
  }
  //printf("handle fork done\n");
}

void create_child(thread_info *parent, thread_info *child) {

  child->user_stack_base = thread_allocate_page(child, STACK_SIZE);
  child->user_program_base = thread_allocate_page(child, USER_PROGRAM_SIZE);
  child->user_program_size = parent->user_program_size;
  parent->child_pid = child->pid; 
  child->child_pid = 0;
  
  init_page_table(child, &(child->pgd));

  for (uint64_t size = 0; size < child->user_program_size; size += PAGE_SIZE) {
    uint64_t virtual_addr = USER_PROGRAM_BASE + size;
    uint64_t physical_addr = VA2PA(child->user_program_base + size);
    update_page_table(child, virtual_addr, physical_addr, 0b110);
  }

  for (uint64_t size = 0; size < PERIPHERAL_END - PERIPHERAL_START; size += PAGE_SIZE) {
    uint64_t identity_page = PERIPHERAL_START + size; 
    // printf("identity_page=%p\n",identity_page);
    update_page_table(child, identity_page , identity_page, 0b110);
  }

  uint64_t virtual_addr = USER_STACK_BASE;
  uint64_t physical_addr = VA2PA(child->user_stack_base);
  update_page_table(child, virtual_addr, physical_addr, 0b110);

  char *src, *dst;
  // copy saved context in thread info
  src = (char *)&(parent->context);
  dst = (char *)&(child->context);
  for (uint32_t i = 0; i < sizeof(cpu_context); ++i, ++src, ++dst) {
    *dst = *src;
  }
  // copy kernel stack
  src = (char *)(parent->kernel_stack_base);
  dst = (char *)(child->kernel_stack_base);
  for (uint32_t i = 0; i < STACK_SIZE; ++i, ++src, ++dst) {
    *dst = *src;
  }
  // copy user stack
  src = (char *)(parent->user_stack_base);
  dst = (char *)(child->user_stack_base);
  for (uint32_t i = 0; i < STACK_SIZE; ++i, ++src, ++dst) {
    *dst = *src;
  }
  // copy user program
  src = (char *)(parent->user_program_base);
  dst = (char *)(child->user_program_base);
  for (uint32_t i = 0; i < parent->user_program_size; ++i, ++src, ++dst) {
    *dst = *src;
  }

  // set correct address for child
  uint64_t kernel_stack_base_dist = child->kernel_stack_base - parent->kernel_stack_base;
  child->context.fp += kernel_stack_base_dist;
  child->context.sp += kernel_stack_base_dist;
  child->trap_frame_addr = parent->trap_frame_addr + kernel_stack_base_dist;

  // with MMU, we do not need to separate user program and stack address
  // uint64_t user_stack_base_dist =
  //     child->user_stack_base - parent->user_stack_base;
  // uint64_t user_program_base_dist =
  //     child->user_program_base - parent->user_program_base;
  // trap_frame_t *trap_frame = (trap_frame_t *)(child->trap_frame_addr);
  // trap_frame->x[29] += user_stack_base_dist;    // fp (x29)
  // trap_frame->x[30] += user_program_base_dist;  // lr (x30)
  // trap_frame->x[32] += user_program_base_dist;  // elr_el1
  // trap_frame->x[33] += user_stack_base_dist;    // sp_el0
}

uint64_t thread_allocate_page(thread_info *thread, uint64_t size) {
  page_frame *page_frame = buddy_allocate(size); // no malloc since the address from malloc is not aligned to 2^12 (wrong page start)
  thread->page_frame_ids[thread->page_frame_count++] = page_frame->id;
  return page_frame->addr;
}

void thread_free_page(thread_info *thread) {
  for (int i = 0; i < thread->page_frame_count; i++) {
    buddy_free(&frames[thread->page_frame_ids[i]]);
  }
}

void switch_pgd(uint64_t next_pgd) {
  asm volatile("dsb ish");  // ensure write has completed
  // we only replace the content in ttbr1_el1 since the ttbr1_el1 will be used for kernel address
  asm volatile("msr ttbr0_el1, %0"
               :
               : "r"(next_pgd));   // switch translation based address.
  asm volatile("tlbi vmalle1is");  // invalidate all TLB entries
  asm volatile("dsb ish");         // ensure completion of TLB invalidatation
  asm volatile("isb");             // clear pipeline
}
// file system related
struct file *thread_get_file(int fd) {
  thread_info *cur = get_current();
  return cur->fd_table.files[fd];
}

int thread_register_fd(struct file *file) {
  if (file == 0) return -1;
  thread_info *cur = get_current();
  // find next available fd
  for (int fd = 3; fd < FD_MAX; ++fd) {
    if (cur->fd_table.files[fd] == 0) {
      cur->fd_table.files[fd] = file;
      return fd;
    }
  }
  return -1;
}

int thread_clear_fd(int fd) {
  if (fd < 0 || fd >= FD_MAX) return -1;
  thread_info *cur = get_current();
  cur->fd_table.files[fd] = 0;
  return 1;
}


void exec() {
  thread_info *cur = get_current();

  if (cur->user_program_base == 0) {
    // thread_allocate_page is just buddy_allocate, which allocates a page.
    // thread_allocate_page also stores additional information about the page, 
    // but it's actually useless.

    cur->user_program_base = buddy_allocate(USER_PROGRAM_SIZE)->addr; //thread_allocate_page(cur, USER_PROGRAM_SIZE);
    cur->user_stack_base = buddy_allocate(STACK_SIZE)->addr; //thread_allocate_page(cur, STACK_SIZE);
    /*
      Why can't you use malloc to allocate pages?
        the reason is simple, the address of malloc is not aligned to the page size, 
        this cause problem since when translating the address, you and page offset with page address,
        if your page address is not xxxxx0000 0000 0000, you'll end you using a corrupted address.

        buddy_allocate() returns the actual aligned page address (see buddy_init)
        it uses page_frame array to store the address of every single pages.

        malloc() uses page, but it stores dma_header in the beginning of the page, and return the 
        address after that, so the address you got is not aligned to 2^12.
    */
    init_page_table(cur, &(cur->pgd)); 
    // assign a page, fill each entry with zero and uses physical address.

    // printf("user_program_base: %x\n", cur->user_program_base);
    // printf("cur_pgd: 0x%llx\n", (uint64_t)(cur->pgd));
    // printf("user program base: 0x%llx\n", cur->user_program_base);
    // printf("user stack base: 0x%llx\n", cur->user_stack_base);
  }

  cur->user_program_size = cpio_load_user_program(exec_program_name, cur->user_program_base);
  // map the user program page tables
  for (uint64_t size = 0; size < cur->user_program_size; size += PAGE_SIZE) {
    uint64_t virtual_addr = USER_PROGRAM_BASE + size;
    uint64_t physical_addr = VA2PA(cur->user_program_base + size);
    update_page_table(cur, virtual_addr, physical_addr, 0b110);
  }
  // map current peripheral addresses to the new user PGD
  for (uint64_t size = 0; size < PERIPHERAL_END - PERIPHERAL_START; size += PAGE_SIZE) {
    uint64_t identity_page = PERIPHERAL_START + size; 
    // printf("identity_page=%p\n",identity_page);
    update_page_table(cur, identity_page , identity_page, 0b110);
  }

  uint64_t virtual_addr = USER_STACK_BASE;
  uint64_t physical_addr = VA2PA(cur->user_stack_base);
  update_page_table(cur, virtual_addr, physical_addr, 0b110);

  uint64_t next_pgd = (uint64_t)cur->pgd;
  switch_pgd(next_pgd);

  uint64_t user_sp = USER_STACK_BASE + STACK_SIZE;

  uint64_t spsr_el1 = 0x0;  // EL0t with interrupt enabled, PSTATE.{DAIF} unmask (0), AArch64 execution state, EL0t
  uint64_t target_addr = USER_PROGRAM_BASE;
  uint64_t target_sp = user_sp;

  //cpio_load_user_program("user_program.img", target_addr);
  //cpio_load_user_program("syscall.img", target_addr);
  //cpio_load_user_program("test_loop", target_addr);
  //cpio_load_user_program("user_shell", target_addr);

  asm volatile("msr spsr_el1, %0" : : "r"(spsr_el1)); // set PSTATE, executions state, stack pointer
  asm volatile("msr elr_el1, %0" : : "r"(target_addr)); // link register at 
  asm volatile("msr sp_el0, %0" : : "r"(target_sp));
  asm volatile("eret"); // eret will fetch spsr_el1, elr_el1.. and jump (return) to user program.
                        // we set the register manually to perform a "jump" or switchning between kernel and user space.
}