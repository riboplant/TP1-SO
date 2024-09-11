// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "../include/include.h"

#define SHM_NAME_LEN 15

int piped_case();
void process_results(char * shm_name);

static int shm_iter = 0;
sem_t * semaphore;
ansT * shm_ptr;

// Para ambos casos hay que refinar como hacer para que cuando termina el hashing de TODOS los archivos, salga del while(1)
int main(int argc, char* argv[]) {
    switch (argc){
    case 1:
        // Me entra por entrada estandar lo de app
        return piped_case();
        break;

    case 2:
        // Me pasaron por parámetro el nombre de la memshare
        process_results(argv[1]);
        break;
    
    default:
        perror("Illegal Arguments");
        break;
    }
}

int piped_case(){

    char shm_name[SHM_NAME_LEN];
    int count;

    if((count = read(STDIN_FILENO, shm_name, SHM_NAME_LEN-1)) == -1){
        perror("Read failed in view: ");
        exit(EXIT_FAILURE);
    }
    shm_name[count] = '\0';

    process_results(shm_name);

    exit(EXIT_SUCCESS);    
}


void process_results(char * shm_name){

    semaphore = sem_open(shm_name, 0);  // No O_CREAT, ya que solo queremos abrir
    if (semaphore == SEM_FAILED) {
        perror("sem_open failed in view:");
        exit(EXIT_FAILURE);
    }

    int shm_fd = shm_open(shm_name, O_RDWR, 0);
    if(shm_fd == -1){
        perror("shm_open failed in view:");
        exit(EXIT_FAILURE);
    }

    struct stat st;
    if (fstat(shm_fd, &st) == -1) {
        perror("fstat failed in view:");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    shm_ptr = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap failed in view:");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    close(shm_fd);
    
    char * to_print;
    int pid;

     while(1){
         // Critical zone 
        sem_wait(semaphore); // Wait till app has written in memshare
        to_print = shm_ptr[shm_iter].md5_name;
        pid = shm_ptr[shm_iter++].pid; 


        if(strcmp(to_print, "STOP READING") == 0){
            break;
        }
        printf("%s   %d\n", to_print, pid);       
     }
     sem_close(semaphore);
     sem_unlink(shm_name);
     if (munmap(shm_ptr, st.st_size) == -1) {
         perror("munmap failed in view: ");
         exit(EXIT_FAILURE);
     }
     shm_unlink(shm_name);
     return;
}