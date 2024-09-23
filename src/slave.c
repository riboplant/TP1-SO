// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "../include/include.h"

int main(void) {
    char path[MAX_PATH_LENGTH];
    int sizePath;

    int pipe_fd = open(PIPE_NAME, O_WRONLY);

    while(1) {

        if(fgets(path, MAX_PATH_LENGTH, stdin) == NULL){
            break;
        }

        if((sizePath = strlen(path)) == 0) {
            perror("empty path");
            close(pipe_fd);
            exit(EXIT_FAILURE);
        }

        // Delete newline character if necessary
        if(path[sizePath - 1] == '\n') {
            path[sizePath - 1] = '\0'; 
        } else {
            path[sizePath] = '\0'; // End the string with '\0' if there is no '\n'
        }

        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("Error al crear el pipe");
            close(pipe_fd);
            exit(EXIT_FAILURE);
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("Error al crear el proceso hijo");
            close(pipe_fd);
            exit(EXIT_FAILURE);
        }
        else if (pid == 0) {

            close(pipefd[0]); 
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]); 
            execlp("md5sum", "md5sum", path, NULL);
            perror("Error al ejecutar md5sum");
            exit(EXIT_FAILURE);

        } 
        else {
            int status;
            waitpid(pid, &status, 0); // Wait for child process
            close(pipefd[1]); // Close the writing end of the pipe

            
            
            char buffer[MAX_PATH_LENGTH+MD5_LENGTH+PID_LENGTH+2]; // 2 extra for spaces
            int count;

            if((count = read(pipefd[0], buffer, sizeof(buffer)-1)) <= 0) {
                close(pipefd[0]); // Close the reading end of the pipe
                perror("MD5 read failed");
                exit(1);
            } else {
                if(buffer[count-1] == '\n'){
                    buffer[count-1] = '\0';
                } else
                    buffer[count] = '\0';

                char buffer2[PID_LENGTH+MAX_PATH_LENGTH+MD5_LENGTH+PID_LENGTH+2];
                sprintf(buffer2, "%s %d", buffer, getpid());
                
                if(write(pipe_fd, buffer2, strlen(buffer2)+1) == -1) {
                    perror("Write to named pipe failed");
                    close(pipe_fd);
                    exit(EXIT_FAILURE);
                }

                printf("%s\n", buffer);
                fflush(stdout);

            }

            close(pipefd[0]); 
            }
    }
    
    close(pipe_fd);
    exit(EXIT_SUCCESS);
}
