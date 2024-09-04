// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "../include/include.h"

// Lo no verificado hasta ahora es el read del pipe del father y el write al pipe (STDOUT_FILENO) y el funcionamiento del select
// El resto funciona, se obtiene perfectamente del path del string, se parsea y se lo envia por un struct.
int main(void) {
    char path[MAX_PATH_LENGTH];
    int sizePath;

    while(1) {

        if((sizePath = read(STDIN_FILENO, path, sizeof(path)-1)) == -1){
            perror("Read failed");
            exit(EXIT_FAILURE);
        } 
        // Si se alcanzó el final de la entrada (EOF), terminó el parseo
        else if(sizePath == 0){
           break;
        }

        // Eliminar el carácter de nueva línea si es necesario
        if(path[sizePath - 1] == '\n') {
            path[sizePath - 1] = '\0'; 
        } else {
            path[sizePath] = '\0'; // Terminar la cadena con '\0' si no hay '\n'
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
            // Código del hijo (child)
            close(pipefd[0]); // Cerrar el extremo de lectura del pipe
            dup2(pipefd[1], STDOUT_FILENO); // Redirigir stdout al pipe
            close(pipefd[1]); // Cerrar el extremo de escritura después de redirigir
            execlp("md5sum", "md5sum", path, NULL);
            perror("Error al ejecutar md5sum");
            exit(EXIT_FAILURE);

        } 
        else {
            wait(NULL); // Esperar al proceso hijo
            // Código del padre (parent)
            close(pipefd[1]); // Cerrar el extremo de escritura del pipe

            // Leer desde el pipe
            char buffer[MAX_PATH_LENGTH+MD5_LENGTH+PID_LENGTH+2]; // 2 extra for spaces
            int count;
            char my_pid[PID_LENGTH];

            if((count = read(pipefd[0], buffer, sizeof(buffer)-1)) <= 0) {
                close(pipefd[0]); // Cerrar el extremo de lectura del pipe
                perror("MD5 read failed");
                exit(1);
            } else {
                if(buffer[count-1] == '\n'){
                    buffer[count-1] = '\0';
                } else
                    buffer[count] = '\0'; // Null-terminar el buffer para imprimir
                sprintf(my_pid, "  %d",getpid());
                strcat(buffer,my_pid);

                printf("%s",buffer);
                fflush(stdout);

            }

            close(pipefd[0]); // Cerrar el extremo de lectura del pipe
            }
    }  

    exit(0);
}
