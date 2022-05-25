#include "thread.h"
#include "timer.h"
#include "string.h"
#include "io.h"
#include "memory.h"
#include "printf.h"
#include "utils.h"
#include "kernel.h"
#include "mmu.h"
void thread_init() {
  run_queue.head = 0;
  run_queue.tail = 0;
  thread_cnt = 0;
}

thread_info *thread_create(void (*func)()) {
  thread_info *thread = (thread_info *)malloc(sizeof(thread_info));
  thread->pid = thread_cnt++;
  thread->status = THREAD_READY;
  thread->next = 0;
  thread->kernel_stack_base = (uint64_t)malloc(STACK_SIZE); // originally 0???
  thread->user_stack_base = (uint64_t)malloc(STACK_SIZE);
  thread->user_program_base =
      USER_PROGRAM_BASE + thread->pid * USER_PROGRAM_SIZE;
  
  thread->context.fp = thread->kernel_stack_base + STACK_SIZE;
  thread->context.lr = (uint64_t)func;
  thread->context.sp = thread->kernel_stack_base + STACK_SIZE;
  thread->user_program_size = USER_PROGRAM_SIZE;
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
  //printf("pid: %d\n", run_queue.head->pid);
  plan_next_interrupt_tval(SCHEDULE_TVAL);
  enable_interrupt();
  
  switch_to((uint64_t)get_current(), run_queue.head);

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
  //disable_interrupt();
  //thread_info *cur = current_thread();
  //cur->status = THREAD_DEAD;
  // kill all thread in this lab, formally, you have to maitain a child pid list and kill child too.
  for (thread_info *ptr = run_queue.head; ptr->next != 0; ptr = ptr->next) {
      ptr->status = THREAD_DEAD;
  }
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
      free((void *)cur);
      ptr->next = tmp;
      cur = tmp;
    }
    if (ptr->next == 0) {
      run_queue.tail = ptr;
      break;
    }
  }
  //printf("[pid:%d] -> ", run_queue.head->pid);
  //printf("\n");
  //enable_interrupt();
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
  printf("creating child malloc\n");
  child->user_stack_base = (uint64_t)malloc(STACK_SIZE);
  child->user_program_size = parent->user_program_size;
  parent->child_pid = child->pid;
  child->child_pid = 0;

  char *src, *dst;
  // copy saved context in thread info
  printf("copying context\n");
  src = (char *)&(parent->context);
  dst = (char *)&(child->context);
  for (uint32_t i = 0; i < sizeof(cpu_context); ++i, ++src, ++dst) {
    *dst = *src;
  }
  // copy kernel stack
  printf("copying kernel stack\n");
  src = (char *)(parent->kernel_stack_base);
  dst = (char *)(child->kernel_stack_base);
  for (uint32_t i = 0; i < STACK_SIZE; ++i, ++src, ++dst) {
    *dst = *src;
  }
  // copy user stack
  printf("copying user stack\n");
  src = (char *)(parent->user_stack_base);
  dst = (char *)(child->user_stack_base);
  for (uint32_t i = 0; i < STACK_SIZE; ++i, ++src, ++dst) {
    *dst = *src;
  }
  // copy user program
  printf("copying user program\n");
  src = (char *)(parent->user_program_base);
  dst = (char *)(child->user_program_base);
  for (uint32_t i = 0; i < parent->user_program_size; ++i, ++src, ++dst) {
    //printf("copying....\n");
    *dst = *src;
  }

  // set correct address for child
  uint64_t kernel_stack_base_dist =
      child->kernel_stack_base - parent->kernel_stack_base;
  uint64_t user_stack_base_dist =
      child->user_stack_base - parent->user_stack_base;
  //printf("kernel_stack_base_dist:%d- %d; %d\n", child->user_stack_base , parent->user_stack_base, kernel_stack_base_dist);
  uint64_t user_program_base_dist =
      child->user_program_base - parent->user_program_base;
  child->context.fp += kernel_stack_base_dist;
  child->context.sp += kernel_stack_base_dist;
  child->trap_frame_addr = parent->trap_frame_addr + kernel_stack_base_dist;

  trap_frame_t *trap_frame = (trap_frame_t *)(child->trap_frame_addr);
  trap_frame->x[29] += user_stack_base_dist;    // fp (x29)
  trap_frame->sp_el0 += user_stack_base_dist;    // sp_el0
  // you don't need to load link register since it's running the same program
  // uses the same program counter to run the same program stored in the memory
  // you don't need following two line to make this lab work, but it's a bit more formal.
  trap_frame->x[30] += user_program_base_dist;    // lr (x30)
  trap_frame->elr_el1 += user_program_base_dist;  // elr_el1
}


void foo() {
  for (int i = 0; i < 4; ++i) {
    //printf("Thread id: %d, %d\r\n", current_thread()->tid, i);
    print_s("Thread id: ");
    print_i(current_thread()->pid);
    print_s("\r\n");
    delay(100000000);
    schedule();
  }
  exit();
  return;
}

void foo2(){
  
  for (int i = 1; i <= 5; ++i) {
    uint64_t lr;
    asm volatile("mov %0, lr" : "=r"(lr));

    printf("link register: %d\n", lr);

    print_s("\r\n");
    print_i(i);
    print_s(",foo2 Thread id: ");
    print_i(current_thread()->pid);
    print_s("\r\n");
    delay(1000); // user small number on real RPi
  }
  //print_s("\n\n\n\ndone!!!!!!\r\n");
  exit();
}

void foo3(){
  while(1){
    printf("foo3\n");
    delay(100000000);
  }
}

//void thread(callback func)

void thread_test() {
  // I'm not sure why you need following lines
  thread_info *idle_t = thread_create(foo3);
  asm volatile("msr tpidr_el1, %0\n" ::"r"((uint64_t)idle_t));
  //for (int i = 0; i < 5; ++i) {
  //  print_i(i);
  //  print_s("\r\n");
  //  thread_create(foo);
  //}
  //thread_create(exec);
  idle();
}

void thread_timer_test(){
  // scheduling using timer interrupt
  print_s("timer schedular test\r\n");
  idle_t = thread_create(0);
  asm volatile("msr tpidr_el1, %0\n" ::"r"((uint64_t)idle_t));

  for (int i = 0; i <5; ++i) {
    print_i(i);
    print_s("\r\n");
    thread_create(foo2);
  }
  bp("start timer\r\n");
  core_timer_enable(SCHEDULE_TVAL);
  plan_next_interrupt_tval(SCHEDULE_TVAL);
  enable_interrupt();
  //idle_thread();
}


void exec() {
    //print_s(args);
   
    //uint64_t target_addr = 0x30100000; // load your program here
    //uint64_t target_sp = 0x31000000;

    thread_info *cur = get_current();
    //if (cur->user_stack_base == 0) {
    //  cur->user_stack_base = (uint64_t)malloc(STACK_SIZE);
    //}
    uint64_t user_sp = cur->user_stack_base + STACK_SIZE;
    cur->user_program_size = USER_PROGRAM_SIZE;
    cpio_load_user_program("syscall.img", cur->user_program_base);

    uint64_t spsr_el1 = 0x0;  // EL0t with interrupt enabled, PSTATE.{DAIF} unmask (0), AArch64 execution state, EL0t
    uint64_t target_addr = cur->user_program_base;
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

void exec_my_user_shell() {
    //print_s(args);
    uint64_t spsr_el1 = 0x0;  // EL0t with interrupt enabled, PSTATE.{DAIF} unmask (0), AArch64 execution state, EL0t
    uint64_t target_addr = KVA + 0x30100000; // load your program here
    uint64_t target_sp = KVA + 0x31000000;

    cpio_load_user_program("user_shell", target_addr);

    asm volatile("msr spsr_el1, %0" : : "r"(spsr_el1)); // set PSTATE, executions state, stack pointer
    asm volatile("msr elr_el1, %0" : : "r"(target_addr)); // link register at 
    asm volatile("msr sp_el0, %0" : : "r"(target_sp));
    asm volatile("eret"); // eret will fetch spsr_el1, elr_el1.. and jump (return) to user program.
                          // we set the register manually to perform a "jump" or switchning between kernel and user space.
}
