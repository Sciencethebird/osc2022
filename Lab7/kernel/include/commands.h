#ifndef __COMMANDS_H
#define __COMMANDS_H


//#include "dtb.h"

typedef struct commads
{
    char* cmd;
    char* help;
    void (*func)(char *);

} commads;

void shell_help(char* args);
void shell_hello(char* args);
void shell_mailbox(char* args);
void shell_reboot(char* args);
void shell_cpio(char* args);
void shell_alloc(char* args);
void shell_user_program(char* args);
void shell_user_program_2(char* args);
void shell_ls(char* args);
void shell_start_timer(char* args);
void shell_async_puts(char* args);
void shell_test(char* args);
void shell_settimeout(char* args);
void shell_events(char* args);
void shell_buddy_test(char* args);
void shell_dma_test(char* args);
void shell_thread_test(char* args);
void shell_thread_timer_test(char* args);
void shell_run(char* args);
void shell_dtb(char* args);
void shell_vfs_test(char* args);

extern commads cmd_list[];
extern int cmd_num;

#endif

