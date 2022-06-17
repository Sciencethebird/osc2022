//#include "start.h"
#include "utils.h"
#include "syscall.h"
#include "printf.h"
#define BUF_SIZE 100

volatile unsigned int  __attribute__((aligned(16))) mbox[36];

void clear_buffer(char* buffer, int size){
    for(int i = 0; i< size; i++){
        buffer[i] ='\0';
    }
}
void print_help(){
    printf("Test command:\n");
    printf("\t1: fork child process test\n");
    printf("\t2: kill forked child process\n");
    printf("\t3: mbox test\n");
}
int main(){

    // print pid of the shell
    int pid = getpid();
    printf("\n[user shell] shell pid = %d\n", pid);
    
    while(1){
        char buff[100];
        clear_buffer(buff, BUF_SIZE);
        printf("# ");
        int read_len = uart_read(buff, 100);
        buff[read_len] = '\0';
        if(buff[0] == '0'){
            print_help();
        }else if(buff[0] == '1'){
            pid = fork();
            if(pid > 0){
                // parent process
                // print frame pointer of the parent process
                int fp;
                asm volatile("mov %0, fp" : "=r"(fp));
                printf("[parent], frame pointer: %d\n", fp);
                // print pid, a variable before pid
                printf("[parent], pid address: %d\n", &pid);
                // parnet pid
                printf("[parent], pid: %d\n", pid);
            }else{
                // child process
                int fp;
                asm volatile("mov %0, fp" : "=r"(fp));
                printf("\n[child], frame pointer: %d\n", fp);
                // print pid, a variable before pid
                printf("[child], pid address: %d\n", &pid);
                // parnet pid
                printf("[child], pid: %d\n", pid);
                while(1){
                    // print frame pointer of the parent process
                    printf("[child], child process loop...\n");
                    
                    delay(1000);
                }
                exit(); // for fail safe
            }
        }else if(buff[0] == '2'){
            printf("[parent] killing child with pid = %d\n", pid);
            kill(pid);
        }else if(buff[0] == '3'){
            printf("[parent] mbox test\n");
            mbox_call(8, mbox);
        }else{
            printf("command no supported\n");
            print_help();
        }
    }
    return 0;
}