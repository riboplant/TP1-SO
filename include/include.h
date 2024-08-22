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

#define MAX_PATH_LENGTH 64
#define MAX_FILE_COUNT 256
#define SLAVE_COUNT 5

typedef char* input;

typedef struct output
{
    pid_t pid;
    char * md5;
    char * file_name;
} output;
