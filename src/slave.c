// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "../include/include.h"

int main(void) {
    char path[MAX_PATH_LENGTH];
    int sizePath;

    while(1) {

        if((sizePath = read(STDIN_FILENO, path, sizeof(path)-1)) == -1){
            perror("Read failed");
            exit(EXIT_FAILURE);
        } 
        // If EOF is reached, stop parsing
        else if(sizePath == 0){
           break;
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
            exit(EXIT_FAILURE);
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("Error al crear el proceso hijo");
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
            wait(NULL); // Wait for child process
            // Parent's code
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

                printf("%s",buffer);
                fflush(stdout);

            }

            close(pipefd[0]); 
            }
    }  

    exit(EXIT_SUCCESS);
}
