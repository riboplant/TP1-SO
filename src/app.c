// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "../include/include.h"

slaveT * create_slaves();
void file_handler(int argc, char * argv[]);
int send_N_to_slave(char * argv[], int fd, int n);
int send_to_slave(char * argv[], int fd);
char *create_semaphore_name(const char *base_name);
void free_resources(int close_pipes);
void close_open_pipes();
void slave_cycle(int max_fd, int argc, char * argv[]);
int get_results(int i);

static int slave_count;
static int read_counter = 0;
static int file_list_iter_write = 1;
static int shm_iter = 0;

slaveT * pipes;

static sem_t * semaphore;

static ansT * shm_ptr;

FILE * output_file; 


int main(int argc, char * argv[]) {
    int ratio = ((argc - 1) / SLAVE_FACTOR);
    
    slave_count = ((ratio > 0) ? ratio : ((argc - 1) / PIPE_FILE_COUNT > 0 ? (argc - 1) / PIPE_FILE_COUNT : 1) );
    slave_count = (slave_count > MAX_SLAVE_COUNT ? MAX_SLAVE_COUNT : slave_count); 

    pipes = create_slaves();

    sem_unlink(SHM_NAME);

    // Set up semaphores
    semaphore = sem_open(SHM_NAME, O_CREAT | O_EXCL, 0777, 0);
    if(semaphore == SEM_FAILED){
        perror("sem_open failed");
        free_resources(TRUE); // TRUE to close open fds 
        exit(EXIT_FAILURE);
    }

    shm_unlink(SHM_NAME);

    // Grab the shared memory block
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0777);
    if(shm_fd == -1){
        perror("shm_open failed with app:");
        free_resources(TRUE);
        exit(EXIT_FAILURE);
    }

    int block_size = ((argc+1) * sizeof(ansT)); 

    if(ftruncate(shm_fd, block_size) == -1){
        perror("ftruncate failed with app:");
        close(shm_fd);
        free_resources(TRUE);    
        exit(EXIT_FAILURE);
    }

    shm_ptr = mmap(NULL, block_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED){
        perror("mmap failed in app:");
        close(shm_fd);
        free_resources(TRUE); 
        exit(EXIT_FAILURE);
    }

    close(shm_fd);

    printf("%s", SHM_NAME);
    fflush(stdout);
    sleep(2);

    output_file = fopen("resultados.txt", "w");  // Open writing file
    if (output_file == NULL) {
        perror("No se pudo abrir el archivo:");
        free_resources(TRUE);
        exit(EXIT_FAILURE);
    }

    file_handler(argc, argv);

    fclose(output_file);

    ansT stopper;
    strcpy(stopper.md5_name, "STOP READING");
    shm_ptr[shm_iter] = stopper;


    // Close entry pipe writing fd when finished parsing
    close_open_pipes();

    //waitpid to prevent zombie processes
    int status;
    for(int i = 0; i < slave_count; i++) {
        if(waitpid(pipes[i].pid, &status, 0) == -1) {
            perror("waipid error");
            exit(EXIT_FAILURE);
        }
    }

    sem_post(semaphore);    // Let view finish if still waiting 


    sem_close(semaphore);
    sem_unlink(SHM_NAME);
    if (munmap(shm_ptr, block_size) == -1) {
        perror("munmap failed");
        free_resources(FALSE); // 0 because there are no fds left open
        exit(EXIT_FAILURE);
    }   
    shm_unlink(SHM_NAME);
    free(pipes);
    exit(0);
 }


/*
Creates a number of slaves specified by SLAVE_COUNT and returns an array of the slaves' PIDs
*/

slaveT * create_slaves() {

    slaveT * pipes = malloc(sizeof(slaveT) * slave_count);
    if(pipes == NULL){
        perror("malloc failed:");
        exit(1);
    }
    
    pid_t cpid;
    for(int i = 0; i < slave_count; i++) {
        // Create pipes
        if(pipe(pipes[i].pipeIn) == -1 || pipe(pipes[i].pipeOut) == -1) {
            perror("pipe failed");
            free(pipes);
            exit(EXIT_FAILURE);
        }
        
        cpid = fork();
        if(cpid == -1) {
            perror("fork error");
            free(pipes);
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
            free(pipes);
            exit(EXIT_FAILURE);
        }

            close(pipes[i].pipeIn[0]); // Close entry pipe reading fd for app
            close(pipes[i].pipeOut[1]); // Close exit pipe writing fd for app
            pipes[i].pid = cpid;    
    }
    return pipes;
}

void file_handler(int argc, char * argv[]) {
    // Check if number of files is lower than number of slaves 
    int min_files = ((slave_count < (argc - 1)) ? slave_count : (argc - 1));
    
    //First cycle, we send PIPE_FILE_COUNT amount of files to each slave 
    for(int i = 0; i < min_files; i++){
        send_N_to_slave(argv, pipes[i].pipeIn[1], PIPE_FILE_COUNT);
    }

    // Search for the largest fd
    int max_fd = 0;
    for(int i = 0; i < slave_count; i++) {
        if(max_fd < pipes[i].pipeOut[0]) {
            max_fd = pipes[i].pipeOut[0];
        }
        if(max_fd < pipes[i].pipeIn[1]) {
            max_fd = pipes[i].pipeIn[1];
        }
    }
    max_fd += 1;

    slave_cycle(max_fd, argc, argv);
}

/*
    Returns -1 on write error or the number of slaves otherwise
*/
int send_N_to_slave(char * argv[], int fd, int n) {
    int ans = -1;
    for(int i = 0; i < n; i++) {
        if((ans = write(fd, argv[file_list_iter_write], strlen(argv[file_list_iter_write]))) == -1){
            perror("Write failed: ");
            free_resources(TRUE);
            exit(EXIT_FAILURE);
        }
        if((ans = write(fd, "\n", 1)) == -1){
            perror("Write failed: ");
            free_resources(TRUE);
            exit(EXIT_FAILURE);
        }
        file_list_iter_write++;  
    }     
    
    return ans;
}

/*
    Reads
*/
void slave_cycle(int max_fd, int argc, char * argv[]) {
    
    fd_set readfds;

    char * toSend;
    char buffer[MAX_PATH_LENGTH + MD5_LENGTH + 1]; 
    int buffer_count;

    while(file_list_iter_write < argc || read_counter < (file_list_iter_write - 1)){
        FD_ZERO(&readfds);

        for(int i = 0; i < slave_count; i++) {
            if(read_counter < (file_list_iter_write - 1))
                FD_SET(pipes[i].pipeOut[0], &readfds);
        }

        int activity = select(max_fd, &readfds, NULL, NULL, NULL);

        if(activity < 0) {
            perror("Error en select()");
            free_resources(TRUE);
            exit(EXIT_FAILURE);
        }

        for(int i = 0; i < slave_count; i++) {
            //read the ouptut from the slave
            if(FD_ISSET(pipes[i].pipeOut[0], &readfds) && read_counter < (file_list_iter_write - 1)) {
                
                //send a new file to the slave
                if(file_list_iter_write < argc) {
                send_N_to_slave(argv, pipes[i].pipeIn[1], 1);
                }

                if((buffer_count = read(pipes[i].pipeOut[0], buffer, sizeof(buffer)-1)) == -1) {
                    perror("Read error");
                    free_resources(TRUE);
                    exit(EXIT_FAILURE);
                } 
                else {
                    buffer[buffer_count] = '\0';
                    toSend = strtok(buffer, "\n");
                    while( toSend != NULL){
                        fprintf(output_file, "%s   %d\n", toSend, pipes[i].pid);

                        ansT ans;
                        strncpy(ans.md5_name, toSend, sizeof(ans.md5_name)-1 );
                        ans.md5_name[sizeof(ans.md5_name)-1] = '\0';
                        ans.pid = pipes[i].pid;

                        // Critical zone
                        shm_ptr[shm_iter++] = ans;
                        sem_post(semaphore);    // Let view read info from memshare

                        read_counter++;
                        toSend = strtok(NULL, "\n");
                    }
                }
            }
        }
    }
}

void free_resources(int close_pipes){
    if(close_pipes){
        close_open_pipes();
    }
    free(pipes);
}

void close_open_pipes(){
   for(int i = 0; i < slave_count; i++){
        close(pipes[i].pipeIn[1]);
        close(pipes[i].pipeOut[0]);
    } 
}
