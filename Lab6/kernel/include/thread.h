#pragma once
#include <stdint.h>
#include <stddef.h>
#include "utils.h"

// thread process setting
#define STACK_SIZE 4096
#define USER_PROGRAM_BASE 0x30000000
#define USER_PROGRAM_SIZE (1 * mb)

// thread status number
#define THREAD_DEAD 1
#define THREAD_FORK 2
#define THREAD_READY 4

// scheduling constant
#define SCHEDULE_TVAL 50000000 >> 5
#define SCHEDULE_TVAL_TEST 62500000 // 1 sec

int from_kernel;
typedef struct {
  uint64_t x19;
  uint64_t x20;
  uint64_t x21;
  uint64_t x22;
  uint64_t x23;
  uint64_t x24;
  uint64_t x25;
  uint64_t x26;
  uint64_t x27;
  uint64_t x28;
  uint64_t fp;
  uint64_t lr;
  uint64_t sp;
} cpu_context;

typedef struct {
  uint64_t x[31];
  uint64_t fp;
  uint64_t lr;
  uint64_t sp;
} cpu_context_full;

typedef struct thread_info {
  //cpu_context context;
  cpu_context context;
  uint32_t pid;
  uint32_t child_pid;
  int status;
  uint64_t trap_frame_addr;
  uint64_t kernel_stack_base;
  uint64_t user_stack_base;
  uint64_t user_program_base;
  uint32_t user_program_size;
  struct thread_info *next;
} thread_info;

typedef struct {
  thread_info *head, *tail;
} thread_queue;

thread_queue run_queue;
uint32_t thread_cnt;
thread_info *idle_t;

extern thread_info *get_current();
extern void switch_to(uint64_t, uint64_t);

typedef struct {
  uint64_t x[31];
  uint64_t spsr_el1;
  uint64_t elr_el1;
  uint64_t sp_el0;
} trap_frame_t;

typedef struct {
  uint64_t x[31];
  uint64_t tpidr_el1;
  uint64_t lr;
  uint64_t sp;
} exception_frame_t;

// thread API
void thread_init();
thread_info *thread_create(void (*func)());
thread_info *current_thread();

// thread API utils
void run_queue_push(thread_info *thread);
void schedule();
void idle();
void idle_thread();
void exit();
void kill(int pid);
void kill_zombies();

// fork 
void fork(uint64_t sp);
void handle_fork();
void create_child(thread_info *parent, thread_info *child);

// timer interrupt schedular
void timer_schedular_handler();

// thread testing functions
void foo();
void foo2();
void thread_test();
void thread_timer_test();


// exec
void exec(); // calls syscall.img
void exec_my_user_shell(); // calls user_shell