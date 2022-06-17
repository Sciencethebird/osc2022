#include "commands.h"
#include "power.h"
#include "mbox.h"
#include "string.h"
#include "io.h"
#include "mini_uart.h"
#include "memory.h"
#include "commands.h"
#include "cpio.h"
#include "timer.h"
#include "thread.h"
#include "dtb.h"
#include "printf.h"

commads cmd_list[]=
{
    {.cmd="help", .help="print this help menu", .func=shell_help},
    {.cmd="hello", .help="print Hello World!", .func=shell_hello},
    {.cmd="reboot", .help="reboot the device", .func=shell_reboot},
    {.cmd="mailbox", .help="get mailbox information", .func=shell_mailbox},
    {.cmd="dtb", .help="dtb test program", .func=shell_dtb},
    {.cmd="cpio", .help="cpio file system demo", .func=shell_cpio},
    {.cmd="ls", .help="list directory", .func=shell_ls},
    {.cmd="alloc", .help="memory allocation test", .func=shell_alloc},
    {.cmd="user", .help="run syscall.img in thread", .func=shell_user_program}, 
    {.cmd="user2", .help="run user_program in thread", .func=shell_user_program_2}, 
    {.cmd="timer-start", .help="start timer", .func=shell_start_timer},
    {.cmd="async-puts", .help="uart async send test", .func=shell_async_puts},
    {.cmd="test", .help="test your command here", .func=shell_test},
    {.cmd="timeout", .help="setTimeout MESSAGE SECONDS", .func=shell_settimeout},
    {.cmd="events", .help="show all timeout events", .func=shell_events},
    {.cmd="buddy", .help="buddy memory system test", .func=shell_buddy_test},
    {.cmd="dma", .help="dynamic memory allocation test", .func=shell_dma_test},
    {.cmd="thread", .help="basic thread function testing", .func=shell_thread_test}, 
    {.cmd="thread-timer", .help="thread scheduling with timer interrrupt", .func=shell_thread_timer_test},
    {.cmd="run", .help="run user program", .func=shell_run}
};

int cmd_num = sizeof(cmd_list)/sizeof(commads);

void shell_cpio(char* args){
    uint64_t cur_addr = CPIO_LOC;
    cpio_newc_header* cpio_ptr;
    uint64_t name_size, file_size;
    char *file_name;
    char *file_content;

    // cpio packet: |header|file_name|data|

    while(1){
        // read the memory section as header
        cpio_ptr = (cpio_newc_header*)cur_addr;
        name_size = hex_to_int64(cpio_ptr->c_namesize);
        file_size = hex_to_int64(cpio_ptr->c_filesize);

        // the file name is after the header file, padded with NUL bytes
        // so the totol size of the header+filename is multiple of 4
        cur_addr += sizeof(cpio_newc_header);
        file_name = (char*)cur_addr;
        if(compare(file_name, "TRAILER!!!")) // special string indicates end of archive
            break;            

        file_content = file_name + name_size; // move pointer to file data location

        uart_puts("File Name: ");
        uart_puts(file_name);
        uart_puts("\r\n");

        for(uint64_t i=0; i<file_size; i++){
            if(file_content[i] == '\n')
                uart_send('\r');
            uart_send(file_content[i]);
        }
            
        // uart_puts(file_content);
        uart_puts("\r\n");
        uart_puts("File Size: ");
        uart_puts(itoa(file_size, 10));
        uart_puts(" bytes");    
        uart_puts("\r\n");
        uart_puts("==========\r\n");

        // file_name is padded with NUL butes, making it multiple of 4
        // add 3 for max possible padding, then and ~3=1111..1100 to cut the last two bits.
        cur_addr = (uint64_t)((cur_addr + name_size + 3) & (~3));
        cur_addr = (uint64_t)((cur_addr + file_size + 3) & (~3));
    }
}

void shell_help(char* args){
    uart_puts("===============================================");
    uart_puts("\r\n");
    uart_puts("Command Name");
    uart_puts("\t");
    uart_puts("Description");
    uart_puts("\r\n");
    uart_puts("===============================================");
    uart_puts("\r\n");

    int cmd_len = sizeof(cmd_list)/sizeof(commads);
    for(int cmd_idx=0; cmd_idx<cmd_len; cmd_idx+=1){
        uart_puts(cmd_list[cmd_idx].cmd);
        uart_puts("\t\t");
        uart_puts(cmd_list[cmd_idx].help);
        uart_puts("\r\n");
    }
    uart_puts("===============================================");
    uart_puts("\r\n");
}

void shell_hello(char* args){
    uart_puts("Hello World!");
}

void shell_reboot(char* args){
    uart_puts("reset");
    reset();
}

void shell_mailbox(char* args){

    // see mailbox detail in https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
    // get the board's unique serial number with a mailbox call
    mbox[0] = 8*4;                  // length of the message
    mbox[1] = MBOX_REQUEST;         // this is a request message
    
    mbox[2] = MBOX_TAG_GETSERIAL;   // get serial number command
    mbox[3] = 8;                    // buffer size
    mbox[4] = 8;
    mbox[5] = 0;                    // clear output buffer
    mbox[6] = 0;

    mbox[7] = MBOX_TAG_LAST;

    // send the message to the GPU and receive answer
    if (mbox_call(MBOX_CH_PROP)) {
        uart_puts("My serial number is: ");
        uart_hex(mbox[6]);
        uart_hex(mbox[5]);
        uart_puts("\r\n");
    } else {
        uart_puts("Unable to query serial!\n");
    }
    
    /* ===== get the board's revison ===== */
    mbox[0] = 8*4;                  // length of the message
    mbox[1] = MBOX_REQUEST;         // this is a request message
    
    mbox[2] = MBOX_TAG_BOARD_REVISION;   // get serial number command
    mbox[3] = 4;                    // buffer size
    mbox[4] = 4;
    mbox[5] = 0;                    // clear output buffer
    mbox[6] = 0;

    mbox[7] = MBOX_TAG_LAST;

    // send the message to the GPU and receive answer
    if (mbox_call(MBOX_CH_PROP)) {
        uart_puts("My board revision is: ");
        uart_hex(mbox[6]);
        uart_hex(mbox[5]);
        uart_puts("\r\n");
    } else {
        uart_puts("Unable to query serial!\n");
    }
    /* ===== get the board's memory info ===== */
    mbox[0] = 8*4;                  // length of the message
    mbox[1] = MBOX_REQUEST;         // this is a request message
    
    mbox[2] = MBOX_TAG_BOARD_MEMORY;   // get serial number command
    mbox[3] = 8;                    // buffer size
    mbox[4] = 8;
    mbox[5] = 0;                    // clear output buffer
    mbox[6] = 0;

    mbox[7] = MBOX_TAG_LAST;

    // send the message to the GPU and receive answer
    if (mbox_call(MBOX_CH_PROP)) {
        uart_puts("My ARM memory base address in bytes is: ");
        uart_hex(mbox[5]);
        uart_puts("\r\n");
        uart_puts("My ARM memory size in bytes is: ");
        uart_hex(mbox[6]);
        uart_puts("\r\n");
    } else {
        uart_puts("Unable to query serial!\n");
    }
    // echo everything back
    // uart_send(uart_getc());
}

void shell_alloc(char* args){
    uart_puts("My Heap starting address in bytes is: ");
    uart_hex(HEAP_START);
    uart_puts("\r\n");

    uart_puts("dtb: ");
    //uart_hex(DTB_ADDR);
    uart_puts("\r\n");


    char* str1 = simple_malloc(8);
    str1 = "apple!\r\n";
    uart_puts(str1);

    char* str2 = simple_malloc(9);
    str2 = "banana!\r\n";
    uart_puts(str2);

    char* array = simple_malloc(sizeof(int)*20);
    for(int i = 0; i< 26; i++){
        array[i] = 'a'+i;
    }
    for(int i = 0; i< 26; i++){
        uart_send(array[i]);
    }
    uart_puts("\r\n");

}

void shell_user_program(char* args){
    printf("user program test:\n");
    idle_t = thread_create(0);
    asm volatile("msr tpidr_el1, %0\n" ::"r"((uint64_t)idle_t));

    thread_create(exec);
    
    core_timer_enable(SCHEDULE_TVAL);
    plan_next_interrupt_tval(SCHEDULE_TVAL);
    enable_interrupt();
    idle();
}

void shell_user_program_2(char* args){
    printf("user 2 program test:\n");
    idle_t = thread_create(0);
    asm volatile("msr tpidr_el1, %0\n" ::"r"((uint64_t)idle_t));
    
    thread_create(exec_my_user_shell);
    
    core_timer_enable(SCHEDULE_TVAL);
    plan_next_interrupt_tval(SCHEDULE_TVAL);
    enable_interrupt();
}


//void 

void shell_ls(char* args){
    cpio_ls();
}

void shell_start_timer(char* args){
    uart_puts("timer enable\n");
    core_timer_enable(-1);
}

void shell_async_puts(char* args){
    enable_uart_interrupt();
    uart_async_puts("async puts test\n");
    disable_uart_interrupt();
}

void shell_test(char* args){
    exec();
}

void shell_settimeout(char* args){
    // parse arg message
    char* timeout_message = args;
    char* ptr = args;
    while((*ptr) != '\0'){
        if((*ptr) == ' ' ){ // message end
            (*ptr) = '\0';
            ptr++;
            break;
        }
        ptr++;
    }
    print_s("timeout message: ");
    print_s(timeout_message);

    int timeout_time = atoi(ptr);
    print_s(", timeout time: ");
    print_i(timeout_time);
    print_s("\n");

    add_timer(print_callback, timeout_message, timeout_time);
}

void shell_run(char* args){
     //print_s(args);
    uint64_t spsr_el1 = 0x0;  // EL0t with interrupt enabled, PSTATE.{DAIF} unmask (0), AArch64 execution state, EL0t
    uint64_t target_addr = 0x30100000; // load your program here
    uint64_t target_sp = 0x31000000;

    //cpio_load_user_program("user_program.img", target_addr);
    cpio_load_user_program(args, target_addr);
    //cpio_load_user_program("test_loop", target_addr);
    //core_timer_enable();
    asm volatile("msr spsr_el1, %0" : : "r"(spsr_el1)); // set PSTATE, executions state, stack pointer
    asm volatile("msr elr_el1, %0" : : "r"(target_addr)); // link register at 
    asm volatile("msr sp_el0, %0" : : "r"(target_sp));
    asm volatile("eret"); // eret will fetch spsr_el1, elr_el1.. and jump (return) to user program.
                          // we set the register manually to perform a "jump" or switchning between kernel and user space.
}

void shell_events(char* args){
    show_all_events();
}

void shell_buddy_test(char* args){
    buddy_test();
}

void shell_dma_test(char* args){
    dma_test();
}

void shell_thread_test(char* args){
    thread_test();
}

void shell_thread_timer_test(char* args){
    thread_timer_test();
}

void shell_dtb(char* args){
    dtb_print(0);
}