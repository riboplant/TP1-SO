#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/file.h>
#include <time.h>
#include <sys/select.h>
#include <fcntl.h>  // Para open
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <semaphore.h>


#define MAX_PATH_LENGTH 128
#define MAX_FILE_COUNT 256
#define MD5_LENGTH 33
#define SLAVE_COUNT 5
#define IPC_RESULT_ERROR (-1)
#define BLOCK_SIZE 4096
//Esta es la que luego haremos variable ? ?
#define FILENAME "memshare"
//filenames for two semaphores
#define SEM_PRODUCER_FNAME "/myproducer"
#define SEM_CONSUMER_FNAME "/myconsumer"


typedef struct output
{
    pid_t pid;
    char * md5;
    char * file_name;
} output;
