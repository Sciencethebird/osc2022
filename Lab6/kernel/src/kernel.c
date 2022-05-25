#include "commands.h"
#include "power.h"
#include "mbox.h"
#include "string.h"
#include "mini_uart.h"
#include "exception.h"
#include "memory.h"
#include "commands.h"
#include "cpio.h"
#include "timer.h"
#include "thread.h"
#include "kernel.h"
//extern commads cmd_list[];

#define BUFFER_LEN 1000

void read_command(char* buffer){
    //core_timer_disable();
    //enable_interrupt();
    enable_uart_interrupt();
    uart_puts("\r\n# ");
    int idx=0;
    while(1){
        if(idx >= BUFFER_LEN) break;
        //char c = uart_getc();
        char c = uart_async_getc();
        if(c=='\n') {
            uart_puts("\r\n"); // echo
            break;
        }
        else {
            buffer[idx++] = c;
        }
        uart_send(c); // echo
    }
    disable_uart_interrupt();
}

void clear_buffer(char* buffer, int size){
    for(int i = 0; i<size; i++){
        buffer[i] = '\0';
    }
}


void execute_command(const char* cmd){
    //int cmd_len = sizeof(cmd_list)/sizeof(commads);
    uart_puts("\0"); // I'dont konw why if you remove this bugs occurs
    int command_found = 0;
    for(int i=0; i<cmd_num; i++){
        if(find_substr(cmd, cmd_list[i].cmd, 0) == 1){
            command_found = 1;
            char *args;
            int args_start = strlen(cmd_list[i].cmd) + 1;
            if(args_start > strlen(cmd)){
                // command with no argument
                args = "foo";
            } else {
                args = (char *)&cmd[args_start];
            }
            
            cmd_list[i].func(args);
            break;
        }
    }
    if(command_found == 0){
        uart_puts("command not found");
    }
}

void run_shell(){
    while(1) {
        // echo everything back
        clear_buffer(input_buffer, BUFFER_LEN);
        // read command
        read_command(input_buffer);
        // uart_puts(input_buffer); // echo input
        execute_command(input_buffer);
    }
}

void main()
{

    // set up serial console
    uart_init();
    // buddy system init
    buddy_init();
    // thread init
    thread_init();
    // say hello
    uart_puts("Wellcome!\r\n");
    // enables interrupt in EL1 (for uart async IO)
    //enable_interrupt(); // msr DAIFClr, 0xf

    head_event = 0;

    enable_interrupt();
    //enable_uart_interrupt();
    run_shell();

    plan_next_interrupt_tval(SCHEDULE_TVAL);
    core_timer_enable(SCHEDULE_TVAL);
    enable_interrupt();
    // once the timer interrupt starts
    // the prgrames enters thread scheduling
    // if there's no other thread to run, the scheduler will call the
    // run_shell function.
}
