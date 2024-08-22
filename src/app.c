#include "../include/include.h"

#define DIRECTORY 8

pid_t * create_slaves(int slaves, int * pipes[][2]);
void parse_dir(char * path, int * pipes[][2]);
int check_pipes(int * pipes[][2]);

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
            if(pipe(pipes[i][j]) == -1) {
                perror("pipe failed");
                exit(EXIT_FAILURE);
            }
        }
    }  
    pid_t * slave_pids = create_slaves(SLAVE_COUNT, pipes);
    parse_dir(argv[1], pipes);
}


/*
Creates a number of slaves specified by SLAVE_COUNT and returns an array of the slaves' PIDs
*/
pid_t * create_slaves(int slaves, int * pipes[][2]) {
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

void parse_dir(char * path, int * pipes[][2]) {
    DIR * dir;
    struct dirent * dp;

    if((dir = opendir(path)) == NULL) {
        perror("path is not a directory");
        return;
    }
    while(((dp = readdir(dir)) != NULL)) {
        if(strncmp(dp->d_name, ".", 1) == 0) {
            continue;
        }

        char new_path[strlen(path) + dp->d_reclen + 1];
        strcpy(new_path, path);
        strcat(new_path, "/");
        strcat(new_path, dp->d_name);

        if(dp->d_type == DIRECTORY) {
            parse_dir(new_path, pipes);
        }
        else { 
            int fd = check_pipes(pipes);
            write(fd, new_path, strlen(new_path));
        }
    }
    closedir(dir);
}


/*
Checks all slave pipes and reads their ouptut, if there is one available. 
Returns the fd of the read pipe
*/
int check_pipes(int * pipes[][2]) {
    fd_set readfds;
    //  fd_set writefds;
    int max_fd = 0;

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

    FD_ZERO(&readfds);              // Limpiar el conjunto de descriptores de escritura
    
    for(int i = 0; i < SLAVE_COUNT; i++) {
        FD_SET(pipes[i][1][1], &readfds);
    }
    // Esperar hasta que alguno de los pipes este listo para lectura
    int activity = select(max_fd, &readfds, NULL, NULL, NULL);
    if(activity < 0) {
        perror("Error en select()");
        exit(EXIT_FAILURE);
    }

    static int isFirstRound = SLAVE_COUNT;

    for(int i = 0; i < SLAVE_COUNT; i++) {
        if(FD_ISSET(pipes[i][1][1], &readfds) || isFirstRound > 0) {
            output * info;
            if(read(pipes[i][1][1], info, sizeof(output)) == -1) {
                printf("name:%s\tmd5:%s\tpid:%d\n", info->file_name , info->md5, info->pid);
                perror("Read error");
                exit(EXIT_FAILURE);
            }
            if(isFirstRound >= 0) {
                isFirstRound--;
            }
            return pipes[i][1][1];
        }
    }
    exit(EXIT_FAILURE);
}