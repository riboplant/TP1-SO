// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "../include/include.h"

#define SHM_NAME_LEN 15

int main() {
    int pipe_fd = open(PIPE_NAME, O_RDONLY);
    
    char to_print[MAX_PATH_LENGTH+MD5_LENGTH+PID_LENGTH+2];

    int count;

     while((count = read(pipe_fd, &to_print, sizeof(to_print))) > 0){
        to_print[count] = '\0';
        printf("%s\n", to_print);       
     }

     close(pipe_fd);
}