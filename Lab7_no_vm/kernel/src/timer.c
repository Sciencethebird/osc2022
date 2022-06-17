#include "timer.h"
#include "thread.h"
#include "memory.h"
#include "io.h"
#include "utils.h"

void core_timer_enable(int tval) {
  asm volatile("mov x0, 1");
  asm volatile("msr cntp_ctl_el0, x0");  // enable interrupt in EL0
  if(tval == -1){ // uses system default frequency * 1 as time interval
    asm volatile("mrs x0, cntfrq_el0"); // system constant
    asm volatile("msr cntp_tval_el0, x0");  // set expired time
  }else{
    plan_next_interrupt_tval(tval);
    //asm volatile("msr cntp_tval_el0, %0" : : "r"(cntfrq_el0 * secs));
  }
  asm volatile("mov x0, 2");
  asm volatile("ldr x1, =0x40000040");
  asm volatile("str w0, [x1]");  // unmask timer interrupt
  //curr_time =0;
  //head_event = 0; // no timeout event yet
  
  // enable cpu timer register for gura video!!!!
  uint64_t tmp;
  asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
  tmp |= 1;
  asm volatile("msr cntkctl_el1, %0" : : "r"(tmp));
}

void core_timer_disable() {
  asm volatile("mov x0, 0");
  asm volatile("msr cntp_ctl_el0, x0");  // disable
  asm volatile("mov x0, 0");
  asm volatile("ldr x1, =0x40000040");
  asm volatile("str w0, [x1]");  // unmask timer interrupt
}

void core_timer_handler() {
  print_s("===== timer handler =====\n");
  uint64_t cntpct_el0, cntfrq_el0;
  asm volatile("mrs %0, cntpct_el0" : "=r"(cntpct_el0));
  asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq_el0));
  asm volatile("mrs x0, cntfrq_el0");
  asm volatile("mov x1, 2");
  asm volatile("mul x0, x0, x1");
  asm volatile("msr cntp_tval_el0, x0");
  print_s("Time elapsed after booting: ");
  print_i(cntpct_el0 / cntfrq_el0);
  print_s("s\n");
}

int get_timestamp(){
  uint64_t cntpct_el0, cntfrq_el0;
  asm volatile("mrs %0, cntpct_el0" : "=r"(cntpct_el0));
  asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq_el0));
  print_s("timestamp now: ");
  print_i(cntpct_el0/cntfrq_el0);
  print_s("\n");
  return cntpct_el0/cntfrq_el0; // maybe use counter?????
}

void timeout_event_handler(){ // EL1-> EL1 irq
  print_s("timout event handler\n");

  // check if head event id valid
  if(head_event ==  0){
    print_s("no timeout event\n");
    return;
  }
  // execute the timeout callback function
  head_event->func(head_event->args);
  head_event = head_event->next_event;
  
  // switch to next timeout event if there's any
  if(head_event == 0){
    core_timer_disable();
    head_event = 0;
    print_s("all events done\n");
  }else{
    // you need to manually set the next timer other wise the tval will
    // go back to the defaul value
    plan_next_interrupt_sec(head_event->queue_time);
  }
}

void plan_next_interrupt_sec(int secs){
  uint64_t cntfrq_el0;
  asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq_el0));
  print_i(cntfrq_el0);
  asm volatile("msr cntp_tval_el0, %0" : : "r"(cntfrq_el0 * secs));
}

void plan_next_interrupt_tval(int tval){
  //print_i(tval);
  asm volatile("msr cntp_tval_el0, %0" : : "r"(tval));
}

void add_timer(callback func, char* args, int duration){
  //func(args);
  timeout_event* new_event = (timeout_event*)malloc(sizeof(timeout_event));
  
  // fill values in the new event
  new_event->func = func;
  new_event->next_event = 0;
  new_event->total_wait_time = duration;
  new_event->queue_time = duration;
  new_event->start_time = get_timestamp();
  for (int i = 0; i < MAX_EVENT_MSG_LEN; i++) {
    new_event->args[i] = args[i];
    if (args[i] == '\0') break;
  }

  // init head event if it's not done yet
  add_timeout_event(new_event);
}

void add_timeout_event(timeout_event* new_event){

  if(head_event == 0){
    print_s(""); // I don't know why you need this line.
    head_event = new_event;
    core_timer_enable(-1); // uses defualt timer frequency
    plan_next_interrupt_sec(new_event->queue_time);
    return;
  }
  // event pointers
  timeout_event* curr_event = head_event;
  timeout_event* prev_event = head_event;

  // update head event wait time
  int curr_time = get_timestamp();
  int head_time_left = head_event->total_wait_time - (curr_time - head_event->start_time);
  head_event->queue_time = head_time_left;

  while(curr_event != 0){
    if((new_event->queue_time) < (curr_event->queue_time)){
      // do insert
      if(curr_event == head_event){
        head_event = new_event;
        new_event->next_event = curr_event;
        // replace the intrrupt
        plan_next_interrupt_sec(head_event->queue_time);
      }else{
        prev_event->next_event = new_event;
        new_event->next_event = curr_event;
      }
      //update_event_time(curr_event, -1 * new_event->queue_time);
      curr_event->queue_time -= new_event->queue_time;
      break;
    }else{
      //print_s("minus wait time\n");
      new_event->queue_time -= curr_event->queue_time;
    }

    if(curr_event->next_event == 0){
      //print_s("new event added");
      curr_event->next_event = new_event;
      break;
    }
    prev_event = curr_event;
    curr_event = curr_event->next_event;
  }
}

void update_event_time(timeout_event* start, int time){
  timeout_event* event_ptr = start;
  while(event_ptr != 0){
    print_s("update events...");
    event_ptr->queue_time += time;
    event_ptr = event_ptr->next_event;
  }
}

void show_all_events(){
  int idx = 0;
  timeout_event* event_ptr = head_event;
  while(event_ptr != 0){
    print_s("[event ");
    print_i(idx++);
    print_s("]: ");
    print_s(" message: ");
    print_s(event_ptr->args);
    print_s(", time: ");
    print_i(event_ptr->queue_time);
    print_s(", original time: ");
    print_i(event_ptr->total_wait_time);
    print_s("\n");
    event_ptr = event_ptr->next_event;
  }
}

void print_callback(char* msg){
  print_s(msg);
  print_s("\n");
}