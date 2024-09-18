#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/select.h>
#include <fcntl.h>  
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>

#define SHM_NAME "/memshare"

#define MAX_PATH_LENGTH 128 
#define MAX_FILE_COUNT 256
#define MD5_LENGTH 33
#define PID_LENGTH 10
#define PIPE_FILE_COUNT 2

#define SLAVE_FACTOR 10
#define MIN_SLAVE_COUNT 5
#define MAX_SLAVE_COUNT 20

#define IPC_RESULT_ERROR (-1)
#define BLOCK_SIZE 4096
#define DEFAULT_SLAVE_FILES 1

#define TRUE 1
#define FALSE 0

typedef struct slave {
    int pipeIn[2];
    int pipeOut[2];
    pid_t pid;
} slaveT;

typedef struct ans {
    char md5_name[MAX_PATH_LENGTH + MD5_LENGTH + 2];
    pid_t pid;
} ansT;