#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define LOCKFILE "/etc/ptmp"

int main(){
    int fds[10];
    for(int i = 0; i<10; i++){
        fds[i] = open("test.txt", O_WRONLY | O_CREAT | O_TRUNC);
        printf("fd: %d\n", fds[i]);
        // you can create multiple file descriptor that points to the same file
        // these file descriptor links to the same file, and they also have same
        // read-write position i.e. fd_pos
    }

    // different file descriptor but same file read-write position, so 
    // the second write() will overwrite the content of the first one
    // since the second write position is also 0 (file start position)
    char content[20] = "hi this is fd3";
    write(fds[3], content, strlen(content));
    char content2[20] = "hi this is fd5";
    write(fds[5], content2, strlen(content2));

    write(fds[3], content, strlen(content));
}
