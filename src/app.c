#include "../include/include.h"

pid_t * create_slaves(int slaves, int * pipes[][]);
void parse_dir(char * path);

int main(int argc, char * argv[]) {

    // "in" and "out" are in reference to the slaves
    int pipe1_in[2], pipe1_out[2], 
        pipe2_in[2], pipe2_out[2], 
        pipe3_in[2], pipe3_out[2], 
        pipe4_in[2], pipe4_out[2], 
        pipe5_in[2], pipe5_out[2];

    int * pipes[SLAVE_COUNT][2] = {{pipe1_in, pipe1_out},
                                 {pipe2_in, pipe2_out},
                                 {pipe3_in, pipe3_out},
                                 {pipe4_in, pipe4_out},
                                 {pipe5_in, pipe5_out}};
                       
    char files[MAX_PATH_LENGTH + 1][MAX_FILE_COUNT] = {};

    if(argc != 2) {
        perror("Illegal arguments error");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < SLAVE_COUNT; i++) {
        for(int j = 0; j < 2; j++) {
            if(pipe(pipes[i][j] == -1)) {
                perror("pipe failed");
                exit(EXIT_FAILURE);
            }
        }
    }  

    pid_t * slave_pids = create_slaves(SLAVE_COUNT, pipes);

    
}


/*
Creates a number of slaves specified by SLAVE_COUNT and returns an array of the slaves' PIDs
*/
pid_t * create_slaves(int slaves, int * pipes[][]) {
    pid_t cpid;
    pid_t ans[slaves];
    for(int i = 0; i < slaves; i++) {
        cpid = fork();
        if(cpid == -1) {
            perror("fork error");
            exit(1);
        }
        else if(cpid == 0) {
            close(pipes[i][0][0]);  // Close entry pipe output 
            dup2(pipes[i][0][1], STDIN_FILENO); // Redirect stdin to entry pipe input
            close(pipes[i][0][1]); // Close original entry pipe input

            close(pipes[i][1][1]);  // Close exit pipe input 
            dup2(pipes[i][1][0], STDOUT_FILENO); // Redirect stdout to exit pipe output
            close(pipes[i][1][0]); // Close original entry pipe input

            char * argv[] = {"slave"};
            char * envp[] = {NULL};
            execve("slave", argv, envp);
            perror("execve error");
        }
        else {
            ans[i] = cpid;   
        }
    }
    return 0;
}

void parse_dir(char * path) {
    DIR * dir;
    struct dirent * dp;

    if((dir = opendir(path)) == NULL) {
        printf("%s is not a directory\n", path);
        return;
    }
    while(((dp = readdir(dir)) != NULL)) {
        if(strncmp(dp->d_name, ".", 1) == 0) {
            continue;
        }
        if(dp->d_type == DIRECTORY) {
            char new_path[strlen(path) + dp->d_reclen + 1];
            strcpy(new_path, path);
            strcat(new_path, "/");
            strcat(new_path, dp->d_name);
            parse_dir(new_path);
        }
    }
    closedir(dir);
}

void check_pipes(int * pipes[][]) {
    fd_set readfds;
    fd_set writefds;
    int max_fd = 0;

    // // Agregar datos de prueba en los pipes
    // write(pipe1[1], "Mensaje desde pipe 1\n", 21);
    // write(pipe2[1], "Mensaje desde pipe 2\n", 21);
    // write(pipe3[1], "Mensaje desde pipe 3\n", 21);

    // Buscamos el descriptor más grande
    for(int i = 0; i < SLAVE_COUNT; i++) {
        for(int j = 0; j < 2; j++) {
            for(int k = 0; k < 2; k++) {
                if(max_fd < pipes[i][j][k]) {
                    max_fd = pipes[i][j][k];
                }
            }  
        }
    }
    max_fd += 1;

    // Bucle para leer datos de los pipes
    while (1) {
        FD_ZERO(&readfds);              // Limpiar el conjunto de descriptores de escritura
        FD_ZERO(&writefds);             // Limpiar el conjunto de descriptores de lectura
        
        for(int i = 0; i < SLAVE_COUNT; i++) {
            FD_SET(pipes[i][j][0], ()&readfds);
            FD_SET(pipes[i][j][1], ()&writefds);
        }

        // Esperar hasta que alguno de los pipes este listo para lectura
        int activity = select(max_fd, &readfds, &writefds, NULL, NULL);

        if(activity < 0) {
            perror("Error en select()");
            break;
        }

        // Verificar si pipe 1 tiene datos disponibles
        if (FD_ISSET(pipes[][0], &readfds)) {
            int bytes_read = read(pipe1[0], buffer, BUFFER_SIZE);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                printf("Datos de pipe 1: %s", buffer);
            }
        }

        // Verificar si pipe 2 tiene datos disponibles
        if (FD_ISSET(pipe2[0], &readfds)) {
            int bytes_read = read(pipe2[0], buffer, BUFFER_SIZE);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                printf("Datos de pipe 2: %s", buffer);
            }
        }

        // Verificar si pipe 3 tiene datos disponibles
        if (FD_ISSET(pipe3[0], &readfds)) {
            int bytes_read = read(pipe3[0], buffer, BUFFER_SIZE);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                printf("Datos de pipe 3: %s", buffer);
            }
        }
    }

    return 0;
}