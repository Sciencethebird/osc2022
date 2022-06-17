#pragma once
#include "utils.h"

#define CORE0_TIMER_IRQ_CTRL 0x40000040
#define MAX_EVENT_MSG_LEN 20

typedef void(*callback)(char *); //message callback
typedef struct timeout_event timeout_event;

struct timeout_event
{
    callback func;
    timeout_event* next_event;
    int total_wait_time;
    int queue_time;
    int start_time;
    char args[MAX_EVENT_MSG_LEN];
};

timeout_event* head_event;

// core timer utils
void core_timer_enable();
void core_timer_disable();
void core_timer_handler();

// timeout event API
void add_timer(callback func, char* args, int duration);
// timeout event API utils
void add_timeout_event(timeout_event* new_event);
void timeout_event_handler();
void plan_next_interrupt_sec(int secs);
void plan_next_interrupt_tval(int tval);
void show_all_events();
void update_event_time(timeout_event* start, int time);

// test callback function
void print_callback(char* msg);

