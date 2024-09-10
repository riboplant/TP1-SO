// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "../include/include.h"
#include <stdlib.h>

typedef struct slave {
    int pipeIn[2];
    int pipeOut[2];
    pid_t pid;
} slaveT;

slaveT * create_slaves();
void file_handler(int argc, char * argv[], slaveT * pipes);
int check_path(char * path, struct stat fileStat);
int cycle_pipes(int argc, char * argv[], slaveT * pipes,  struct stat fileStat);
void get_results(slaveT * pipes);
int send_N_to_slave(char * argv[], int fd, slaveT * pipes, struct stat fileStat, int n);
int send_to_slave(char * argv[], int fd, slaveT * pipes, struct stat fileStat);

// static int get_shared_block(char* filename, int size);
// void * attach_memory_block(char* filename, int size);
// int detach_memory_block(char* block);
// int destroy_memory_block(char* filename);
// void write_shmem(char * data, int data_size);

static int file_list_iter = 1;
static int slave_count;

sem_t * sem_prod;
sem_t * sem_cons;

typedef struct shmemBlock {
    char * shmblock;
    int block_size;
    int used_space;
} shmemBlockT; 

int main(int argc, char * argv[]) {
    int ratio = ((argc - 1) / SLAVE_FACTOR);
    
    slave_count = ((ratio > 0) ? ratio : ((argc - 1) / PIPE_FILE_COUNT > 0 ? (argc - 1) / PIPE_FILE_COUNT : 1) );
    slave_count = (slave_count > MAX_SLAVE_COUNT ? MAX_SLAVE_COUNT : slave_count); 

    slaveT * pipes = create_slaves();

    

    // sem_unlink(SEM_PRODUCER_FNAME);
    // sem_unlink(SEM_CONSUMER_FNAME);

    // Set up semaphores
    // sem_prod = sem_open(SEM_PRODUCER_FNAME, O_CREAT | O_EXCL, 0644, 0);
    // if(sem_prod == SEM_FAILED){
    //     perror("sem_open/producer failed");
    //     exit(EXIT_FAILURE);
    // }

    // sem_cons = sem_open(SEM_CONSUMER_FNAME, O_CREAT | O_EXCL, 0644, 0);
    // if(sem_cons == SEM_FAILED){
    //     perror("sem_open/consumer failed");
    //     exit(EXIT_FAILURE);
    // }

    //Agregar cuando se lee e imprime: antes de imprimir
    // Grab the shared memory block
    // block_size = ((argc+1) * (MAX_PATH_LENGTH + MD5_LENGTH + PID_LENGTH + 3));
    // shmblock = attach_memory_block(FILENAME, block_size);
    // if(shmblock == NULL){
    //     printf("ERROR: couldn't get block\n");
    //     return -1;
    // }


    file_handler(argc, argv, pipes);


    // Close entry pipe writing fd when finished parsing
    for(int i = 0; i < slave_count; i++){
        close(pipes[i].pipeIn[1]);
        close(pipes[i].pipeOut[0]);
    } 




//  sem_wait(sem_prod); // Wait for the consumer to have an open slot.

    // Ya fuera del ciclo, luego de terminar
    // sem_close(sem_prod);
    // sem_close(sem_cons);
    // detach_memory_block(shmblock);
    // destroy_memory_block(FILENAME);
    free(pipes);
    exit(0);
}


/*
Creates a number of slaves specified by SLAVE_COUNT and returns an array of the slaves' PIDs
*/
slaveT * create_slaves() {

    slaveT * pipes = malloc(sizeof(slaveT) * slave_count);
    
    pid_t cpid;
    for(int i = 0; i < slave_count; i++) {
        // Create pipes
        if(pipe(pipes[i].pipeIn) == -1 || pipe(pipes[i].pipeOut) == -1) {
            perror("pipe failed");
            exit(EXIT_FAILURE);
        }
        
        cpid = fork();
        if(cpid == -1) {
            perror("fork error");
            exit(1);
        }
        else if(cpid == 0) {
            //Close any other fd 
            for(int j = 0; j <= i; j++){
                close(pipes[j].pipeOut[0]); 
                close(pipes[j].pipeIn[1]);
            }
            
            dup2(pipes[i].pipeIn[0], STDIN_FILENO); // Redirect stdin to entry pipe input
            close(pipes[i].pipeIn[0]); // Close original entry pipe input

            dup2(pipes[i].pipeOut[1], STDOUT_FILENO); // Redirect stdout to exit pipe output
            close(pipes[i].pipeOut[1]); // Close original entry pipe output

            char * argv[] = {"slave"};
            char * envp[] = {NULL};
            execve("slave", argv, envp);
            perror("execve error");
            exit(1);
        }

            close(pipes[i].pipeIn[0]); // Close entry pipe reading fd for app
            close(pipes[i].pipeOut[1]); // Close exit pipe writing fd for app
            pipes[i].pid = getpid();    
    }
    return pipes;
}

void file_handler(int argc, char * argv[], slaveT * pipes) {
    int min_files = ((slave_count < (argc - 1)) ? slave_count : (argc - 1));
    struct stat fileStat;

    //primera pasada, le paso PIPE_FILE_COUNT a cada slave para arrancar
    //TODO: se puede modularizar haciendo que cycle_pipes reciba cuantos archivos pasarle a cada pipe
    for(int i = 0; i < min_files; i++){
        send_N_to_slave(argv, pipes[i].pipeIn[1], pipes, fileStat, PIPE_FILE_COUNT);
    }

    while(file_list_iter < argc) {
        cycle_pipes(argc, argv, pipes, fileStat);
    }
}


// int send_to_slave(char * argv[], int fd, slaveT * pipes, struct stat fileStat) {

//   //  sem_wait(sem_prod); // Wait for view.c to consume from buffer

//     int c, ans = -1;
//     if((c = check_path(argv[file_list_iter], fileStat)) == -1){
//                 perror("Invalid path");
//                 exit(EXIT_FAILURE);
//         } else if(c == 0){
//             if((ans = write(fd, argv[file_list_iter], strlen(argv[file_list_iter]))) == -1){
//                 perror("Write failed: ");
//                 exit(EXIT_FAILURE);
//             }
//             else {
//                 get_results(pipes);
//             }
//         }
//     file_list_iter++;
//     return ans;
// }
/*
    Returns -1 on write error or the number of slaves otherwise
*/
int send_N_to_slave(char * argv[], int fd, slaveT * pipes, struct stat fileStat, int n) {
    int ans = -1;
    for(int i = 0; i < n; i++) {
        int c = -1;
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
    }
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
int cycle_pipes(int argc, char * argv[], slaveT * pipes, struct stat fileStat) {

  //  sem_wait(sem_prod); // Wait for view.c to consume from buffer

    fd_set writefds;
    int max_fd = 0;

    // Buscamos el descriptor más grande
    for(int i = 0; i < slave_count; i++) {
        if(max_fd < pipes[i].pipeIn[1]) {
            max_fd = pipes[i].pipeIn[1];
        }
    }
    max_fd += 1;

    FD_ZERO(&writefds);  // Limpiar el conjunto de descriptores de escritura
    
    for(int i = 0; i < slave_count; i++) {
        FD_SET(pipes[i].pipeIn[1], &writefds);
    }

    // Esperar hasta que alguno de los pipes este listo para lectura
    int activity = select(max_fd, NULL, &writefds, NULL, NULL);

    if(activity < 0) {
        perror("Error en select()");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < slave_count && (file_list_iter < argc); i++) {
        if(FD_ISSET(pipes[i].pipeIn[1], &writefds)){
            send_N_to_slave(argv, pipes[i].pipeIn[1], pipes, fileStat, 1);      //Envío el archivo al fd de escritura del pipe de entrada
        }
    }
    
    return(0);
}

void get_results(slaveT * pipes){

    fd_set readfds;
    int max_fd = 0;

    // Buscamos el descriptor más grande
    for(int i = 0; i < slave_count; i++) {
        if(max_fd < pipes[i].pipeOut[0]) {
            max_fd = pipes[i].pipeOut[0];
        }
    }
    max_fd += 1;

    FD_ZERO(&readfds);

    for(int i = 0; i < slave_count; i++) {
        FD_SET(pipes[i].pipeOut[0], &readfds);
    }

    int activity = select(max_fd, &readfds, NULL, NULL, NULL);

    if(activity < 0) {
        perror("Error en select()");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < slave_count; i++) {

        if(FD_ISSET(pipes[i].pipeOut[0], &readfds)) {
            char buffer[MAX_PATH_LENGTH + MD5_LENGTH + PID_LENGTH + 3]; // 3 extra spaces for spaces and '\0'
            int count;
            // char md5_hash[MD5_LENGTH]; // Buffer para el hash MD5 (32 caracteres + 1 para '\0')
            // char filename[MAX_PATH_LENGTH]; // Buffer para el nombre del archivo
            if((count = read(pipes[i].pipeOut[0], buffer, sizeof(buffer)-1)) == -1) {
                perror("Read error");
                exit(EXIT_FAILURE);
            } 
            else {
               buffer[count] = '\0';
               if(write(STDOUT_FILENO,buffer,count+1) == -1){
                    perror("Write failed");
                    exit(1);
               }
            //   sem_post(sem_cons); // Signal view.c buffer is ready to be read

              // write_shmem(buffer,count+1);
            //    strcat(buffer,cpid);
            //    printf("%s\n", buffer);
             }
        }
    }
}

// A partir de aca estan las funciones auxiliares de memshare

// static int get_shared_block(char* filename, int size){
//     FILE *file = fopen(filename, "w");
//     if (file == NULL) {
//         perror("Failed to create temp file");
//         exit(EXIT_FAILURE);
//     }
//     fclose(file);

//     // Request a key, the key is linked to a filename, so that other programs can access it.
//     key_t key= ftok(filename, 0);
//     if(key == IPC_RESULT_ERROR) return IPC_RESULT_ERROR;

//     unlink(filename);

//     //Get shared block --- create it if it does not exist
//     return shmget(key, size, 0644 | IPC_CREAT);
// }

// void * attach_memory_block(char* filename, int size){
//     int shared_block_id = get_shared_block(filename, size);
//     char* result;

//     if(shared_block_id == IPC_RESULT_ERROR) {
//         return NULL;
//     }

//     // Map the shared block into this process's memory and give me a pointer to it
//     result = shmat(shared_block_id, NULL, 0);
//     if(result == (void *)IPC_RESULT_ERROR){
//         return NULL;
//     }
//     return result;
// }

// int detach_memory_block(char * block){
//     return (shmdt(block) != IPC_RESULT_ERROR);
// }

// int destroy_memory_block(char * filename){
//     int shared_blok_id = get_shared_block(filename, 0);

//     if(shared_blok_id == IPC_RESULT_ERROR) return -1;

//     return (shmctl(shared_blok_id, IPC_RMID, NULL) != IPC_RESULT_ERROR);
// }

// void write_shmem(char * data, int data_size){
//     // Verificar si hay suficiente espacio en el bloque
//     if (used_space + data_size > block_size) {
//         printf("No hay suficiente espacio en la memoria compartida.\n");
//         return;
//     }

//     // Escribir datos en la memoria compartida
//     memcpy(shmblock + used_space, data, data_size);
//     used_space += data_size;  // Actualizar el espacio utilizado
// }
