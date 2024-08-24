#include "../include/include.h"

// Lo no verificado hasta ahora es el read del pipe del father y el write al pipe (STDOUT_FILENO) y el funcionamiento del select
// El resto funciona, se obtiene perfectamente del path del string, se parsea y se lo envia por un struct.
int main2(void);

int main2(void){
    char path[128];
    int sizePath;
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    int activity = select(STDIN_FILENO+1, &readfds, NULL, NULL, NULL);
    if(activity < 0) {
        perror("Error en select()");
        exit(EXIT_FAILURE);
    }

    if((sizePath = read(STDIN_FILENO, path, sizeof(path)-1)) == -1){
        perror("Read failed");
        exit(1);
    }

    // Si se alcanzó el final de la entrada (EOF), no agregar nada
    if(sizePath == 0){
        printf("EOF reached, no input provided.\n");
    } else {
        // Eliminar el carácter de nueva línea si es necesario
        if(path[sizePath - 1] == '\n'){
            path[sizePath - 1] = '\0';
        } else {
            path[sizePath] = '\0'; // Terminar la cadena con '\0' si no hay '\n'
        }
    }

    printf("Path: %s\n", path);
    return 0;
}

int main(void) {
    char path[128];
    int sizePath;
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    while(1) {
    int activity = select(STDIN_FILENO+1, &readfds, NULL, NULL, NULL);
    if(activity < 0) {
        perror("Error en select()");
        exit(EXIT_FAILURE);
    }

    if((sizePath = read(STDIN_FILENO, path, sizeof(path)-1)) == -1){
        perror("Read failed");
        exit(1);
    }

    // Si se alcanzó el final de la entrada (EOF), no agregar nada
    if(sizePath == 0){
        printf("EOF reached, no input provided.\n");
    } else {
        // Eliminar el carácter de nueva línea si es necesario
        if(path[sizePath - 1] == '\n'){
            path[sizePath - 1] = '\0';
        } else {
            path[sizePath] = '\0'; // Terminar la cadena con '\0' si no hay '\n'
        }
    }


    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("Error al crear el pipe");
        exit(1);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("Error al crear el proceso hijo");
        exit(1);
    } else if (pid == 0) {
        // Código del hijo (child)
        close(pipefd[0]); // Cerrar el extremo de lectura del pipe
        dup2(pipefd[1], STDOUT_FILENO); // Redirigir stdout al pipe
        close(pipefd[1]); // Cerrar el extremo de escritura después de redirigir
        execlp("md5sum", "md5sum", path, NULL);
        perror("Error al ejecutar md5sum");
        exit(1);
    } else {
        wait(NULL); // Esperar al proceso hijo
        // Código del padre (parent)
        close(pipefd[1]); // Cerrar el extremo de escritura del pipe

        // Leer desde el pipe
        char buffer[128];
        ssize_t count;
        char md5_hash[33]; // Buffer para el hash MD5 (32 caracteres + 1 para '\0')
        char filename[96]; // Buffer para el nombre del archivo

        if((count = read(pipefd[0], buffer, sizeof(buffer) - 1)) <= 0) {
            perror("MD5 read failed");
            exit(1);
        } else {
            buffer[count] = '\0'; // Null-terminar el buffer para imprimir
            printf("Buffer: %s", buffer); // TESTING MD5 PIPES
            //Parsear la salida para extraer el hash MD5 y el nombre del archivo
            // if (sscanf(buffer, "%32s %95s", md5_hash, filename) == 2) {

            //     output parsed_data;
            //     parsed_data.file_name = filename;
            //     parsed_data.md5 = md5_hash;
            //     parsed_data.pid = getpid();

            //     if(write(STDOUT_FILENO, &parsed_data, sizeof(output)) == -1){
            //         perror("Write failed: ");
            //         exit(1);
            //     }
            // } else {
            //     fprintf(stderr, "Error al parsear la salida\n");
            // }
        }

        close(pipefd[0]); // Cerrar el extremo de lectura del pipe
    }
    exit(0);
    }
}
