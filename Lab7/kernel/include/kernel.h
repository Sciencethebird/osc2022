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

//extern commads cmd_list[];

#define BUFFER_LEN 1000

char input_buffer[BUFFER_LEN];
void read_command(char* buffer);
void clear_buffer(char* buffer, int size);
void execute_command(const char* cmd);
void run_shell();