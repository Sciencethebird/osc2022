#include <stdint.h>
#include "exception.h"
#include "mbox.h"
#include "thread.h" 
#include "io.h"
#include "mini_uart.h"
#include "timer.h"
#include "printf.h"
#include "utils.h"
#include "mmu.h"
#include "thread.h"


void enable_interrupt() { asm volatile("msr DAIFClr, 0xf"); }
void disable_interrupt() { asm volatile("msr DAIFSet, 0xf"); }

void currentEL_ELx_sync_handler() {
  printf("[sync_handler_currentEL_ELx]\n");

  uint64_t spsr_el1, elr_el1, esr_el1;
  asm volatile("mrs %0, spsr_el1" : "=r"(spsr_el1));
  asm volatile("mrs %0, elr_el1" : "=r"(elr_el1));
  asm volatile("mrs %0, esr_el1" : "=r"(esr_el1));
  printf("[SPSR_EL1]: 0x%08x\n", spsr_el1);
  printf("[ELR_EL1]: 0x%08x\n", elr_el1);
  printf("[ESR_EL1]: 0x%08x\n", esr_el1);
  int ec = (esr_el1 >> 26) & 0x3f; // exception class
  printf("[ec]: %d\n", ec);
  if(ec == 37){
    printf("[PID]: %d\n", get_current()->pid);
    printf("[ERROR]: MMU faults generated by data accesses\n");
  }

  while(1){}
  // 0x96000000 = 10010110000000000000000000000000
}

void lower_64_EL_sync_handler(uint64_t sp){

  // printf("trap frame sp: %x\n", sp);

  // exception registor
  uint64_t spsr_el1, elr_el1, esr_el1;
  asm volatile("mrs %0, spsr_el1" : "=r"(spsr_el1));
  asm volatile("mrs %0, elr_el1" : "=r"(elr_el1));
  asm volatile("mrs %0, esr_el1" : "=r"(esr_el1));

  // https://developer.arm.com/documentation/ddi0595/2021-06/AArch64-Registers/ESR-EL1--Exception-Syndrome-Register--EL1-

  int ec = (esr_el1 >> 26) & 0x3f; // exception class

  if(ec == 0b010101){
    // according the lab spec, the number of the systme call is store in x8
    trap_frame_t *trap_frame = (trap_frame_t *)sp;
    int sys_call_num;
    asm volatile("mov %0, x8" : "=r"(sys_call_num));
    //printf("sys_call_num = %d\n", sys_call_num);

    if(sys_call_num == 0){ // int getpid()

      uint64_t pid = get_current()->pid;
      trap_frame->x[0]=pid;

    }else if(sys_call_num == 1){ // uart_read(char buf[], size_t size)

      disable_uart_interrupt(); // I'm not sure why
      char *str = (char *)(trap_frame->x[0]);
      uint32_t size = (uint32_t)(trap_frame->x[1]);
      
      enable_interrupt();
      size = uart_gets(str, size);

      trap_frame->x[0] = size;
      
    }else if(sys_call_num == 2){ // uart_write(char buf[], size_t size)

      // print_s("uart_write called\r\n");
      disable_uart_interrupt();
      char* str = (char*) trap_frame->x[0];
      uint32_t size = (uint32_t)(trap_frame->x[1]);

      uart_write(str, size);  

    }else if(sys_call_num == 3){

      print_s("exec called\r\n");
      exec();

    }else if(sys_call_num == 4){

      print_s("fork called\r\n");
      fork(sp);

    }else if(sys_call_num == 5){

      print_s("exit called\r\n");
      exit();

    }else if(sys_call_num == 6){

      print_s("mbox_call called\r\n");
      
      unsigned char ch = (unsigned char) trap_frame->x[0];
      unsigned int *mbox_user_VA = (unsigned int*)(trap_frame->x[1]);

      // manual address translation： user VA --walk--> user PA --PA2VA--> kernel PA --> 
      unsigned int *mbox_user_PA = user_VA2PA(get_current(), (uint64_t)mbox_user_VA);
      unsigned int *mbox_kernel_VA = (unsigned int *) PA2VA(mbox_user_PA);
      printf("mbox ch: %d, *mbox: %d\n", ch, mbox_kernel_VA);

      int valid = mbox_call_user(ch, mbox_kernel_VA);

      trap_frame->x[0] = valid;
      // printf("mbox call valid: %d\n", valid);

    }else if(sys_call_num == 7){
      
      print_s("kill called\r\n");
      int pid = trap_frame->x[0];
      kill(pid);

    }else{

      print_s("unhandled system call number\r\n");

    }
  }
  //printf("pid %d done exception handling\n", get_current());
}

// user timer handler
void el0_to_el1_irq_handler() {
  disable_interrupt();
  uint32_t is_uart = (*IRQ_PENDING_1 & AUX_IRQ);
  uint32_t is_core_timer = (*CORE0_IRQ_SOURCE & CNTPNS_IRQ);
  //printf("is_core_timer: %d\n", is_core_timer);
  if (is_uart) {
    uart_handler();
  } else if(is_core_timer) {
    timer_schedular_handler();
  }
  enable_interrupt();
}

// kernel timer handler
uint64_t el1_to_el1_irq_handler(uint64_t sp) {
  disable_interrupt();
  uint32_t is_uart = (*IRQ_PENDING_1 & AUX_IRQ);
  uint32_t is_core_timer = (*CORE0_IRQ_SOURCE & CNTPNS_IRQ);
  //printf("kernel:is_core_timer: %d\n", is_core_timer);
  if (is_uart) {
    uart_handler();
  } else if(is_core_timer) {
    schedule();
  }
  //plan_next_interrupt_tval(SCHEDULE_TVAL);
  enable_interrupt();
  return sp;
}

void default_handler() { print_s("===== default handler =====\n"); }