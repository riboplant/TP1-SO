#include "../include/include.h"

#define DIRECTORY 8

void create_slaves(int * pipes[][2], pid_t * pids);
void parse_dir(char * path, int * pipes[][2], struct stat fileStat);
int check_pipes(int * pipes[][2]);
void get_results(int * pipes[][2]);

int main(int argc, char * argv[]) {

    if(argc != 2) {
        perror("Invalid arguments error");
        exit(EXIT_FAILURE);
    }
    
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
                       
    // char files[MAX_PATH_LENGTH + 1][MAX_FILE_COUNT] = {};

    for(int i = 0; i < SLAVE_COUNT; i++) {
        for(int j = 0; j < 2; j++) {
            if(pipe(pipes[i][j]) == -1) {
                perror("pipe failed");
                exit(EXIT_FAILURE);
            }
        }
    }

    pid_t slave_pids[SLAVE_COUNT];
    create_slaves(pipes, slave_pids);

    struct stat fileStat;
    parse_dir(argv[1], pipes, fileStat);

    for(int i=0; i<SLAVE_COUNT; i++){
        close(pipes[i][0][1]);
        kill(slave_pids[i],SIGTERM);
    }  
    exit(0);
}


/*
Creates a number of slaves specified by SLAVE_COUNT and returns an array of the slaves' PIDs
*/
void create_slaves(int * pipes[][2], pid_t * pids) {
    pid_t cpid;
    for(int i = 0; i < SLAVE_COUNT; i++) {
        cpid = fork();
        if(cpid == -1) {
            perror("fork error");
            exit(1);
        }
        else if(cpid == 0) {
            close(pipes[i][0][1]);  // Close entry pipe output 
            dup2(pipes[i][0][0], STDIN_FILENO); // Redirect stdin to entry pipe input
            close(pipes[i][0][0]); // Close original entry pipe input

            close(pipes[i][1][0]);  // Close exit pipe input 
            dup2(pipes[i][1][1], STDOUT_FILENO); // Redirect stdout to exit pipe output
            close(pipes[i][1][1]); // Close original entry pipe output

            char * argv[] = {"slave"};
            char * envp[] = {NULL};
            execve("slave", argv, envp);
            perror("execve error");
        }
        else {
            close(pipes[i][0][0]); // Close entry pipe reading fd for app
            close(pipes[i][1][1]); // Close exit pipe writing fd for app
            // TESTING PIPES:
            // char* str = "./app";
            // write(pipes[i][0][1],str,strlen(str)); 
            // close(pipes[i][0][1]);
            
            // waitpid(cpid,NULL,0);

            // char buff[128];
            // int count;
            // if((count = read(pipes[i][1][0],buff,sizeof(buff)-1)) > 0){
            //     buff[count] = '\0';
            //     printf("%s\n",buff);
            // }
            // close(pipes[i][1][0]);
            pids[i] = cpid;
        }
    }
    return;
}

void parse_dir(char * path, int * pipes[][2], struct stat fileStat) {
    
    if(stat(path, &fileStat) < 0){
        perror(path);
        return;
    }

    if(S_ISDIR(fileStat.st_mode)){
        DIR * dir;
        struct dirent * dp;

        if((dir = opendir(path)) == NULL) {
            perror("Open failed: ");
            return;
        }

        while(((dp = readdir(dir)) != NULL)) {
            if(strncmp(dp->d_name, ".", 1) == 0) 
                continue;

            char new_path[strlen(path) + dp->d_reclen + 1];
            strcpy(new_path, path);
            strcat(new_path, "/");
            strcat(new_path, dp->d_name);

            parse_dir(new_path, pipes,fileStat);
        }

        closedir(dir);

    } else if(S_ISREG(fileStat.st_mode)){
        int fd = check_pipes(pipes);

        if(write(fd, path, strlen(path)) == -1){
            perror("Write failed: ");
            return;
        }

        get_results(pipes);
    }

    return;
}


/*
Checks all slave pipes and reads their ouptut, if there is one available. 
Returns the fd of the read pipe
*/
int check_pipes(int * pipes[][2]) {

    static int isFirstRound = SLAVE_COUNT;

    if(isFirstRound > 0) {
        isFirstRound--;
        return pipes[SLAVE_COUNT-(isFirstRound+1)][0][1];
    }


    fd_set writefds;
    int max_fd = 0;

    // Buscamos el descriptor más grande
    for(int i = 0; i < SLAVE_COUNT; i++) {
        if(max_fd < pipes[i][0][1]) {
            max_fd = pipes[i][0][1];
        }
    }
    max_fd += 1;

    FD_ZERO(&writefds);  // Limpiar el conjunto de descriptores de escritura
    
    for(int i = 0; i < SLAVE_COUNT; i++) {
        FD_SET(pipes[i][0][1], &writefds);
    }

    // Esperar hasta que alguno de los pipes este listo para lectura
    int activity = select(max_fd, NULL, &writefds, NULL, NULL);

    if(activity < 0) {
        perror("Error en select()");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < SLAVE_COUNT; i++) {
        if(FD_ISSET(pipes[i][0][1], &writefds)){
            return pipes[i][0][1]; // Retorno el fd de escritura del pipe de entrada
        }
    }
    exit(EXIT_FAILURE);
}


void get_results(int * pipes[][2]){

    fd_set readfds;
    int max_fd = 0;

    // Buscamos el descriptor más grande
    for(int i = 0; i < SLAVE_COUNT; i++) {
        if(max_fd < pipes[i][1][0]) {
            max_fd = pipes[i][1][0];
        }
    }
    max_fd += 1;

    FD_ZERO(&readfds);

    for(int i = 0; i < SLAVE_COUNT; i++) {
        FD_SET(pipes[i][1][0], &readfds);
    }

    int activity = select(max_fd, &readfds, NULL, NULL, NULL);

    if(activity < 0) {
        perror("Error en select()");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < SLAVE_COUNT; i++) {

        if(FD_ISSET(pipes[i][1][0], &readfds)) {
            char buffer[MAX_PATH_LENGTH];
            int count;
            char md5_hash[MD5_LENGTH]; // Buffer para el hash MD5 (32 caracteres + 1 para '\0')
            char filename[MAX_PATH_LENGTH]; // Buffer para el nombre del archivo
            pid_t cpid;
            if((count = read(pipes[i][1][0], buffer, sizeof(buffer))) == -1) {
                perror("Read error");
                exit(EXIT_FAILURE);
            } 
            else {
               buffer[count] = '\0';
               //Parsear la salida para extraer el hash MD5 y el nombre del archivo
                if (sscanf(buffer, "%32s %128s %d", md5_hash, filename, &cpid) == 3) {

                output parsed_data;
                parsed_data.file_name = filename;
                parsed_data.md5 = md5_hash;
                parsed_data.pid = cpid;
                
                printf("path:%s\t\tmd5:%s\tpid:%d\n", parsed_data.file_name , parsed_data.md5, parsed_data.pid);
                }
                else {
                    perror("Error al parsear la salida\n");
                }  
            }
        }
    }
    
}