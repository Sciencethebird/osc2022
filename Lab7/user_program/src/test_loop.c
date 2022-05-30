#include "start.h"
#include "printf.h"
//void read_command()
int main(){
    print_s("User Process: test loop\r\n");
    char buff[1000];
    int pid = fork();
    print_s("pid after fork:");
    print_i(pid);
    print_s("\r\n");

    print_s("pid location: \r\n");
    print_i(&pid);
    print_s("\r\n");
    // %d\n", pid);
    if(pid >0){
        
        while(1){
            int child_pid = pid;

            int fp;
            asm volatile("mov %0, fp" : "=r"(fp));
            print_s("parent fp: ");
            print_i(fp);

            print_s("parent pid: ");
            //delay(1000000000);
            print_i(getpid());
            print_s("\r\n");
            int len = uart_read(buff, 100);
            print_s("read result: \r\n");
            uart_write(buff, len);
            print_s("\r\n");
            if(buff[0] == '0'){
                print_s("target pid: ");
                print_i(pid);
                print_s("kill child process\r\n");
                
                kill(pid);
            }
            //exit();
        }
    }else{
        while(1){
            //int len = uart_read(buff, 100);

            int fp;
            asm volatile("mov %0, fp" : "=r"(fp));
            print_s("child fp: ");
            print_i(fp);



            print_s("child pid: ");
            //delay(1000000000);
            print_i(getpid());
            print_s("\r\n");
            delay(100000000);
        }
    }
    return 0;
}