#include "../include/include.h"

int piped_case();
int memory_shared(char* arg);
static int get_shared_block(char* filename, int size);
char* attach_memory_block(char* filename, int size);
int detach_memory_block(char* block);
int destroy_memory_block(char* filename);

// Para ambos casos hay que refinar como hacer para que cuando termina el hashing de TODOS los archivos, salga del while(1)
int main(int argc, char* argv[]) {
    switch (argc){
    case 1:
        // Me entra por entrada estandar lo de app
        return piped_case();
        break;

    case 2:
        // Me pasaron por argumentos el id del memshare
        memory_shared(argv[1]);
        break;
    
    default:
        perror("Illegal Arguments");
        break;
    }


}



int piped_case(){
    fd_set fdentry;
    FD_ZERO(&fdentry);
    FD_SET(STDIN_FILENO, &fdentry);

    char buffer[MAX_PATH_LENGTH + MD5_LENGTH + PID_LENGTH + 3];
    int count;

    while(1){
        select(STDIN_FILENO + 1, &fdentry, NULL, NULL, NULL);
        if((count = read(STDIN_FILENO, buffer, strlen(buffer))) == -1){
            perror("Read failed: ");
            return 1;
        }
        
        if (count == 0) {
            // End of input (EOF)
            break;
        } 
        if(write(STDOUT_FILENO, buffer, count) == -1){
            perror("Write failed: "); 
            return 1; 
        }
    }
    return 0;
}

int memory_shared(char* shmem_block){
    // Set up semaphores
    sem_unlink(SEM_CONSUMER_FNAME);
    sem_unlink(SEM_PRODUCER_FNAME);

    sem_t* sem_prod = sem_open(SEM_PRODUCER_FNAME, O_CREAT, 0660, 0);
    if(sem_prod == SEM_FAILED){
        perror("sem_open/producer falied");
        exit(EXIT_FAILURE);
    }

    sem_t* sem_cons = sem_open(SEM_CONSUMER_FNAME, O_CREAT, 0660, 1);
    if(sem_cons == SEM_FAILED){
        perror("sem_open/consumer falied");
        exit(EXIT_FAILURE);
    }

    //Grab the shared memory block
    char * block = attach_memory_block(shmem_block, BLOCK_SIZE);
    if(block == NULL){
        printf("ERROR: couldnt get block\n");
        return -1;
    }

    while(1){
        sem_wait(sem_prod);

        if(strlen(block) > 0){
            printf("READING: \"%s\"\n", block);
            int done = (strcmp(block, "quit") == 0);
            block[0] = 0;
            if(done) {break;}
        }

        sem_post(sem_cons);
    }


    sem_close(sem_prod);
    sem_close(sem_cons);
    detach_memory_block(block);

    return 0;
}

// A partir de aca estan las funciones auxiliares para compartir memoria

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
    if(result == (char*)IPC_RESULT_ERROR) return NULL;

    return result;
}

int detach_memory_block(char* block){
    return (shmdt(block) != IPC_RESULT_ERROR);
}

int destroy_memory_block(char* filename){
    int shared_blok_id = get_shared_block(filename, 0);

    if(shared_blok_id == IPC_RESULT_ERROR) return -1;

    return (shmctl(shared_blok_id, IPC_RMID, NULL) != IPC_RESULT_ERROR);
}