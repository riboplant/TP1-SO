// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "../include/include.h"

#define SHM_NAME_LEN 15

int piped_case();
int param_case(char* arg);
// static int get_shared_block(char* filename, int size);
// char* attach_memory_block(char* filename, int size);
// int detach_memory_block(char* block);
// int destroy_memory_block(char* filename);

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
        // Me pasaron por argumentos el id del memshare
        param_case(argv[1]);
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

    puts(shm_name);

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
         // Zona crítica
        sem_wait(semaphore);
        to_print = shm_ptr[shm_iter].md5_name;
        pid = shm_ptr[shm_iter++].pid; 
        sem_post(semaphore);

        if(strcmp(to_print, "STOP READING") == 0){
            break;
        }
        printf("%s   %d\n", to_print, pid);       
     }
     sem_close(semaphore);
     sem_unlink(shm_name);
     if (munmap(shm_ptr, st.st_size) == -1) {
         perror("munmap failed in view: ");
         exit(1);
     }
     shm_unlink(shm_name);
     exit(EXIT_SUCCESS);    
}

int param_case(char* shmem_block){
//     // Set up semaphores
//     sem_unlink(shmem_block);

//     sem_t* sem_prod = sem_open(shmem_block, O_CREAT, 0660, 0);
//     if(sem_prod == SEM_FAILED){
//         perror("sem_open/producer falied");
//         exit(EXIT_FAILURE);
//     }

//     sem_t* sem_cons = sem_open(shmem_block, O_CREAT, 0660, 1);
//     if(sem_cons == SEM_FAILED){
//         perror("sem_open/consumer falied");
//         exit(EXIT_FAILURE);
//     }

//     //Grab the shared memory block
//     char * block = attach_memory_block(shmem_block, BLOCK_SIZE);
//     if(block == NULL){
//         printf("ERROR: couldnt get block\n");
//         return -1;
//     }

//     while(1){
//         sem_wait(sem_prod);

//         if(strlen(block) > 0){
//             printf("READING: \"%s\"\n", block);
//             int done = (strcmp(block, "quit") == 0);
//             block[0] = 0;
//             if(done) {break;}
//         }

//         sem_post(sem_cons);
//     }


//     sem_close(sem_prod);
//     sem_close(sem_cons);
//     detach_memory_block(block);

     return 0;
 }

// A partir de aca estan las funciones auxiliares para compartir memoria

// static int get_shared_block(char* filename, int size){
//     key_t key;

//     // Request a key, the key is linked to a filename, so that other programs can access it.
//     key= ftok(filename, 0);
//     if(key == IPC_RESULT_ERROR) return IPC_RESULT_ERROR;

//     //Get shared block --- create it if it does not exist
//     return shmget(key, size, 0644 | IPC_CREAT);
// }

// char* attach_memory_block(char* filename, int size){
//     int shared_block_id = get_shared_block(filename, size);
//     char* result;

//     if(shared_block_id == IPC_RESULT_ERROR) return NULL;

//     // Map the shared blovk into this process's memory and give me a pointer to it
//     result = shmat(shared_block_id, NULL, 0);
//     if(result == (char*)IPC_RESULT_ERROR) return NULL;

//     return result;
// }

// int detach_memory_block(char* block){
//     return (shmdt(block) != IPC_RESULT_ERROR);
// }

// int destroy_memory_block(char* filename){
//     int shared_blok_id = get_shared_block(filename, 0);

//     if(shared_blok_id == IPC_RESULT_ERROR) return -1;

//     return (shmctl(shared_blok_id, IPC_RMID, NULL) != IPC_RESULT_ERROR);
// }