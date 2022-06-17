#ifndef __COMMANDS_H
#define __COMMANDS_H

#define CAT_BUF_SIZE 4096
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
void shell_cpio_ls(char* args);
void shell_async_puts(char* args);
void shell_test(char* args);
void shell_settimeout(char* args);
void shell_events(char* args);
void shell_buddy_test(char* args);
void shell_dma_test(char* args);
void shell_dtb(char* args);
void shell_vfs_test(char* args);
void shell_ls(char* args);
void shell_chdir(char* args);
void shell_cat(char* args);

extern commads cmd_list[];
extern int cmd_num;

#endif

