#include "../include/include.h"

void create_slaves(int * pipes[][2], pid_t * pids);
void file_handler(int argc, char * argv[], int * pipes[][2]);
int check_path(char * path, struct stat fileStat);
int cycle_pipes(char * argv[], int argc, int * pipes[][2],  struct stat fileStat);
void get_results(int * pipes[][2]);
int send_to_slave(char * argv[], int fd, int * pipes[][2], struct stat fileStat);

static int get_shared_block(char* filename, int size);
char* attach_memory_block(char* filename, int size);
int detach_memory_block(char* block);
int destroy_memory_block(char* filename);

static int file_list_iter = 1;

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

    // Set up semaphores
    // sem_t* sem_prod = sem_open(SEM_PRODUCER_FNAME, 0);
    // if(sem_prod == SEM_FAILED){
    //     perror("sem_open/producer falied");
    //     exit(EXIT_FAILURE);
    // }

    // sem_t* sem_cons = sem_open(SEM_CONSUMER_FNAME, 0);
    // if(sem_cons == SEM_FAILED){
    //     perror("sem_open/consumer falied");
    //     exit(EXIT_FAILURE);

        file_handler(argc, argv, pipes);


    //     //Agregar cuando se lee e imprime: antes de imprimir
    //     // Grab the shared memory block
    //     char* block = attach_memory_block(FILENAME, BLOCK_SIZE);
    //     if(block == NULL){
    //         printf("ERROR: couldn't get block\n");
    //         return -1;
    //     }
    //     sem_wait(sem_cons); // Wait for the consumer to have an open slot.
    //     // Escribo y luego:
    //     sem_post(sem_prod); // Signal tht something has been produced

    //     // Ya fuera del ciclo, luego de terminar
    //     sem_close(sem_prod);
    //     sem_close(sem_cons);
    //     detach_memory_block(block);
    //     destroy_memory_block(FILENAME);

    //     //Kill the slaves when finished
    //     for(int i=0; i<SLAVE_COUNT; i++){
    //         close(pipes[i][0][1]);
    //         kill(slave_pids[i],SIGTERM);
    //     }  
    //     exit(0);
    // }

    for(int i=0; i<SLAVE_COUNT; i++){
        close(pipes[i][0][1]);
        kill(slave_pids[i],SIGTERM);
    }  
    
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

void file_handler(int argc, char * argv[], int * pipes[][2]) {
    int min_files = ((SLAVE_COUNT < (argc - 1)) ? SLAVE_COUNT : (argc - 1));
    struct stat fileStat;

    //primera pasada, le paso PIPE_FILE_COUNT a cada slave para arrancar
    for(int i = 0; i < min_files; i++ ) {
        for(int j = 0; j < PIPE_FILE_COUNT && file_list_iter <= (argc - 1); j++) {
            send_to_slave(argv, pipes[i][0][1], pipes, fileStat);   
        }
    }
    while(file_list_iter < argc) {
        cycle_pipes(argv, argc, pipes, fileStat);
    }
}

/*
    Returns -1 on write error or the number written otherwise
*/
int send_to_slave(char * argv[], int fd, int * pipes[][2], struct stat fileStat) {
    int c, ans = -1;
    if((c = check_path(argv[file_list_iter], fileStat)) == -1){
                perror("Invalid path");
                exit(EXIT_FAILURE);
        } else if(c == 0){
            if((ans = write(fd, argv[file_list_iter], strlen(argv[file_list_iter]))) == -1){
                perror("Write failed: ");
                exit(EXIT_FAILURE);
            }
            else {
                get_results(pipes);
            }
        }
    file_list_iter++;
    return ans;
}

/*
    Returns 0 if path is a FILE, 1 if path is a DIR, -1 in any other case. 
*/
int check_path(char * path, struct stat fileStat){
    
    if(stat(path, &fileStat) < 0){
        perror(path);
        return -1;
    }

    if(S_ISDIR(fileStat.st_mode)){
        return 1;
    } 

    return S_ISREG(fileStat.st_mode) - 1;
}


/*
    Cycles through all slave pipes and reads their ouptut, if there is one available. 
    Passes file paths to every slave's pipe.
*/
int cycle_pipes(char * argv[], int argc, int * pipes[][2], struct stat fileStat) {
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

    for(int i = 0; i < SLAVE_COUNT && (file_list_iter < argc); i++) {
        if(FD_ISSET(pipes[i][0][1], &writefds)){
            send_to_slave(argv, pipes[i][0][1], pipes, fileStat);      //Envío el archivo al fd de escritura del pipe de entrada
        }
    }
    
    return(0);
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
                if (sscanf(buffer, "%32s %1024s %d", md5_hash, filename, &cpid) == 3) {

                output parsed_data;
                parsed_data.file_name = filename;
                parsed_data.md5 = md5_hash;
                parsed_data.pid = cpid;
                
                printf("path:%s\t\tmd5:%s\tpid:%d\n", parsed_data.file_name , parsed_data.md5, parsed_data.pid);
                }
                else {
                    perror("Error al parsear la salida\n");
                    exit(EXIT_FAILURE);
                }  
            }
        }
    }
    
}

// A partir de aca estan las funciones auxiliares de memshare

static int get_shared_block(char* filename, int size){
    key_t key;

    // Request a key, the key is linked to a filename, so that other programs can access it.
    key= ftok(filename, 0);
    if(key == IPC_RESULT_ERROR) return IPC_RESULT_ERROR;

    //Get shared block --- create it if it does not exist
    return shmget(key, size, 0644 | IPC_CREAT);
}

char* attach_memory_block(char* filename, int size){
    int shared_block_id = get_shared_block(filename, size);
    char* result;

    if(shared_block_id == IPC_RESULT_ERROR) return NULL;

    // Map the shared blovk into this process's memory and give me a pointer to it
    result = shmat(shared_block_id, NULL, 0);
    if(result == (char *)IPC_RESULT_ERROR) return NULL;

    return result;
}

int detach_memory_block(char * block){
    return (shmdt(block) != IPC_RESULT_ERROR);
}

int destroy_memory_block(char * filename){
    int shared_blok_id = get_shared_block(filename, 0);

    if(shared_blok_id == IPC_RESULT_ERROR) return -1;

    return (shmctl(shared_blok_id, IPC_RMID, NULL) != IPC_RESULT_ERROR);
}
