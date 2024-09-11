// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "../include/include.h"

slaveT * create_slaves();
void file_handler(int argc, char * argv[], slaveT * pipes);
int check_path(char * path, struct stat fileStat);
int cycle_pipes(int argc, char * argv[], slaveT * pipes,  struct stat fileStat);
void get_results(slaveT * pipes);
int send_N_to_slave(char * argv[], int fd, slaveT * pipes, struct stat fileStat, int n);
int send_to_slave(char * argv[], int fd, slaveT * pipes, struct stat fileStat);
char *create_semaphore_name(const char *base_name);

static int slave_count;
static int file_list_iter = 1;
static int shm_iter = 0;

static sem_t * semaphore;

static ansT * shm_ptr;

 FILE * output_file; 


int main(int argc, char * argv[]) {
    int ratio = ((argc - 1) / SLAVE_FACTOR);
    
    slave_count = ((ratio > 0) ? ratio : ((argc - 1) / PIPE_FILE_COUNT > 0 ? (argc - 1) / PIPE_FILE_COUNT : 1) );
    slave_count = (slave_count > MAX_SLAVE_COUNT ? MAX_SLAVE_COUNT : slave_count); 

    slaveT * pipes = create_slaves();

    char* key = create_semaphore_name(SEM_NAME_BASE);

    sem_unlink(key);

    // Set up semaphores
    semaphore = sem_open(key, O_CREAT | O_EXCL, 0777, 0);
    if(semaphore == SEM_FAILED){
        perror("sem_open/producer failed");
        exit(EXIT_FAILURE);
    }

    shm_unlink(key);

    // Grab the shared memory block
    int shm_fd = shm_open(key, O_CREAT | O_EXCL | O_RDWR, 0777);
    if(shm_fd == -1){
        perror("shm_open failed with app:");
        exit(1);
    }

    int block_size = ((argc+1) * sizeof(ansT)); 

    if(ftruncate(shm_fd, block_size) == -1){
        close(shm_fd);
        perror("ftruncate failed with app:");
        exit(1);
    }

    shm_ptr = mmap(NULL, block_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED){
        close(shm_fd);
        perror("mmap failed in app:");
        exit(1);
    }


    printf("%s", key);
    fflush(stdout);
    sleep(5);

    output_file = fopen("resultados.txt", "w");  // Open writing file
    if (output_file == NULL) {
        perror("No se pudo abrir el archivo:");
        return 1;
    }

    file_handler(argc, argv, pipes);

    fclose(output_file);

    ansT stopper;
    strcpy(stopper.md5_name, "STOP READING");
    shm_ptr[shm_iter] = stopper;


    // // Close entry pipe writing fd when finished parsing
    for(int i = 0; i < slave_count; i++){
        close(pipes[i].pipeIn[1]);
        close(pipes[i].pipeOut[0]);
    } 

    sem_post(semaphore);    // Let view finish if still waiting 


    sem_close(semaphore);
    sem_unlink(key);
    if (munmap(shm_ptr, block_size) == -1) {
        perror("munmap failed");
        exit(1);
    }   
    shm_unlink(key);
    free(pipes);
    free(key);
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
            pipes[i].pid = cpid;    
    }
    return pipes;
}

void file_handler(int argc, char * argv[], slaveT * pipes) {
    // Check if number of files is lower than number of slaves 
    int min_files = ((slave_count < (argc - 1)) ? slave_count : (argc - 1));
    struct stat fileStat;

    //primera pasada, le paso PIPE_FILE_COUNT a cada slave para arrancar
    for(int i = 0; i < min_files; i++){
        send_N_to_slave(argv, pipes[i].pipeIn[1], pipes, fileStat, PIPE_FILE_COUNT);
    }

    while(file_list_iter < argc) {
        cycle_pipes(argc, argv, pipes, fileStat);
    }
}

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
            char buffer[MAX_PATH_LENGTH + MD5_LENGTH + 1]; 
            int count;

            if((count = read(pipes[i].pipeOut[0], buffer, sizeof(buffer)-1)) == -1) {
                perror("Read error");
                exit(EXIT_FAILURE);
            } 
            else {
               buffer[count] = '\0';

               fprintf(output_file, "%s   %d\n", buffer, pipes[i].pid);

               ansT ans;
               strncpy(ans.md5_name, buffer, sizeof(ans.md5_name)-1 );
               ans.md5_name[sizeof(ans.md5_name)-1] = '\0';
               ans.pid = pipes[i].pid;

               // Critical zone
                shm_ptr[shm_iter++] = ans;
                sem_post(semaphore);    // Let view read info from memshare
             }
        }
    }
}


char *create_semaphore_name(const char *base_name){
    char *sem_name = malloc(MAX_NAME_LEN);
    if (!sem_name) {
        return NULL;  // Error en la asignación de memoria
    }

    int suffix = 0;

    // Intentar generar un nombre hasta que no haya uno ya existente
    while (1) {
        if (suffix == 0) {
            snprintf(sem_name, MAX_NAME_LEN, "%s", base_name);
        } else {
            snprintf(sem_name, MAX_NAME_LEN, "%s_%d", base_name, suffix);
        }

        sem_t *test_sem = sem_open(sem_name, O_CREAT | O_EXCL, 0644, 1);
        if (test_sem == SEM_FAILED) {
            if (errno == EEXIST) {
                // Si ya existe, intentar con otro nombre añadiendo un sufijo numérico
                suffix++;
            } else {
                // Otro error que no sea EEXIST, falló
                free(sem_name);
                return NULL;
            }
        } else {
            // Se encontró un nombre que no existe, cerramos el semáforo de prueba
            sem_close(test_sem);
            sem_unlink(sem_name);  // Deshacemos la creación temporal
            return sem_name;       // Retornamos el nombre disponible
        }
    }
}