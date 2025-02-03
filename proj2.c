#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/mman.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>
#include <fcntl.h>
#include <semaphore.h> // for semaphore-related functions
#include <pthread.h>

// Structure for shared memory
typedef struct {
    int clerk_count;
    int customer_count;
    int max_waiting_time;
    int max_break_time;
    int max_closure_time;
    _Bool post_opened;
    int action_number;
    int service1;
    int service2;
    int service3;
} SharedData;

typedef struct {
    char* name;
    int value;
    char identifier[19]; 
} SemaphoreInfo;

// Declaration of semaphores

sem_t* xkised00_output;
sem_t* xkised00_closure;
sem_t* xkised00_service_1;
sem_t* xkised00_service_2;
sem_t* xkised00_service_3;
sem_t* xkised00_clerk;

int validateValue(int value, const char* valueName, int minValue, int maxValue) {
    if (value < minValue || value > maxValue) {
        fprintf(stderr, "%s is out of range. Valid range: %d <= %s <= %d\n", valueName, minValue, valueName, maxValue);
        return 1;
    }
    return 0;
}

int initialize_semaphores() {
    xkised00_clerk = (sem_t *) mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    xkised00_output = (sem_t *) mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    xkised00_closure = (sem_t *) mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    xkised00_service_1 = (sem_t *) mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    xkised00_service_2 = (sem_t *) mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    xkised00_service_3 = (sem_t *) mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    // Initialize semaphores
    if (sem_init(xkised00_output, -1, 1) == -1) {
        fprintf(stderr, "Failed to initialize semaphore xkised00_output\n");
        exit(1);
    }

    if (sem_init(xkised00_closure, -1, 1) == -1) {
        fprintf(stderr, "Failed to initialize semaphore xkised00_closure\n");
        exit(1);
    }

    if (sem_init(xkised00_service_1, -1, 0) == -1) {
        fprintf(stderr, "Failed to initialize semaphore xkised00_service_1\n");
        exit(1);
    }

    if (sem_init(xkised00_service_2, -1, 0) == -1) {
        fprintf(stderr, "Failed to initialize semaphore xkised00_service_2\n");
        exit(1);
    }

    if (sem_init(xkised00_service_3, -1, 0) == -1) {
        fprintf(stderr, "Failed to initialize semaphore xkised00_service_3\n");
        exit(1);
    }

    if (sem_init(xkised00_clerk, -1, 1) == -1) {
        fprintf(stderr, "Failed to initialize semaphore xkised00_clerk\n");
        exit(1);
    }

    return 0;
}



void cleanup() {
    // Define the semaphore identifiers
    char identifiers[6][19] = {
        "xkised00_service_1",
        "xkised00_service_2",
        "xkised00_service_3",
        "xkised00_clerk",
        "xkised00_output",
        "xkised00_closure"
    };

    for (int i = 0; i < 6; i++) {
        key_t sem_key = ftok(".", identifiers[i][0]);
        int sem_id = semget(sem_key, 0, 0);
        if (sem_id != -1) {
            // Remove the semaphore
            if (semctl(sem_id, 0, IPC_RMID) == -1) {
                fprintf(stderr, "Failed to remove semaphore for identifier %s\n", identifiers[i]);
            }
        }
    }
}

void customer_process(int idZ, SharedData* sharedData, FILE* file) {
    srand(time(NULL) ^ (getpid()<<16));
    // Start of the customer process
    sem_wait(xkised00_output); // Lock the semaphore for output management
    fprintf(file, "%d: Z %d: started\n", ++(sharedData->action_number), idZ);
    fflush(file);
    sem_post(xkised00_output); // Unlock the semaphore for output management
    // Wait for a random time interval
    int waiting_time = rand() % (sharedData->max_waiting_time + 1);
    // Sleep for the calculated waiting time
    usleep(waiting_time * 1000);
    sem_wait(xkised00_closure);
    if (sharedData->post_opened == false) {
        // Post office is closed
        sem_wait(xkised00_output); // Lock the semaphore for output management
        fprintf(file, "%d: Z %d: going home\n", ++(sharedData->action_number), idZ);
        fflush(file);
        sem_post(xkised00_output); // Unlock the semaphore for output management
        sem_post(xkised00_closure);
        // End the process
        exit(0);
        
    }else {
        // Post office is open
        
        // Randomly select a service number (1, 2, or 3)
        int serviceNumber = (rand() % 3) + 1;

        sem_wait(xkised00_output); // Lock the semaphore for output management
        fprintf(file, "%d: Z %d: entering office for a service %d\n", ++(sharedData->action_number), idZ, serviceNumber);
        fflush(file);
        sem_post(xkised00_output); // Unlock the semaphore for output management
        sem_post(xkised00_closure);
        // Wait for the office worker to call
        if(serviceNumber == 1){
            (sharedData->service1)++;
            sem_wait(xkised00_service_1);
        }else if (serviceNumber == 2){
            (sharedData->service2)++;
            sem_wait(xkised00_service_2);
        }else{
            (sharedData->service3)++;
            sem_wait(xkised00_service_3);
        }
        sem_wait(xkised00_output); // Lock the semaphore for output management
        fprintf(file, "%d: Z %d: called by office worker\n", ++(sharedData->action_number), idZ);
        fflush(file);
        sem_post(xkised00_output); // Unlock the semaphore for output management
        // Wait for a random time interval
        int randsleep = (rand() % 11);
        usleep(randsleep * 1000);
        sem_wait(xkised00_output); // Lock the semaphore for output management
        fprintf(file, "%d: Z %d: going home\n", ++(sharedData->action_number), idZ);
        fflush(file);
        sem_post(xkised00_output); // Unlock the semaphore for output management
        // End the process
        exit(0);
    }
}

void clerk_process(int idU, SharedData* sharedData, FILE* file) {
    srand(time(NULL) ^ (getpid()<<16));
    // Start of the clerk process
    sem_wait(xkised00_output); // Lock the semaphore for output management
    fprintf(file, "%d: U %d: started\n", ++(sharedData->action_number), idU);
    fflush(file);
    sem_post(xkised00_output); // Unlock the semaphore for output management
    int hodnota = 0;
    while(true){
        hodnota = 0;
        sem_wait(xkised00_clerk);
        if(((sharedData->service1 == 0)&&(sharedData->service2 == 0)&&(sharedData->service3 == 0))){
            sem_post(xkised00_clerk);
            if(sharedData->post_opened == false){
                sem_wait(xkised00_output); // Lock the semaphore for output management
                fprintf(file, "%d: U %d: going home\n", ++(sharedData->action_number), idU);
                fflush(file);
                sem_post(xkised00_output); // Unlock the semaphore for output management
                exit(0);
            }
            sem_wait(xkised00_output); // Lock the semaphore for output management
            fprintf(file, "%d: U %d: taking break\n", ++(sharedData->action_number), idU);
            fflush(file);
            sem_post(xkised00_output); // Unlock the semaphore for output management
            int waiting_time = rand() % (sharedData->max_break_time + 1);
            // on a break for the calculated waiting time
            usleep(waiting_time * 1000);
            sem_wait(xkised00_output); // Lock the semaphore for output management
            fprintf(file, "%d: U %d: break finished\n", ++(sharedData->action_number), idU);
            fflush(file);
            sem_post(xkised00_output); // Unlock the semaphore for output management
        }else{
            while( hodnota == 0 ){
                // Select a service number (1, 2, or 3) randomly
                int serviceNumber = (rand() % 3) + 1;
                if((serviceNumber == 1)&&((sharedData->service1)>0)){
                    --(sharedData->service1);
                    hodnota = serviceNumber;
                    sem_post(xkised00_service_1);
                }else if ((serviceNumber == 2)&&((sharedData->service2)>0)){
                    --(sharedData->service2);
                    hodnota = serviceNumber;
                    sem_post(xkised00_service_2);
                }else if((serviceNumber == 3)&&((sharedData->service3)>0)){
                    --(sharedData->service3);
                    hodnota = serviceNumber;
                    sem_post(xkised00_service_3);
                }
            }
            sem_post(xkised00_clerk);
            sem_wait(xkised00_output); // Lock the semaphore for output management
            fprintf(file, "%d: U %d: serving a service of type %d\n", ++(sharedData->action_number), idU, hodnota);
            fflush(file);
            sem_post(xkised00_output); // Unlock the semaphore for output management
            int randsleep = (rand() % 11);
            usleep(randsleep * 1000);
            sem_wait(xkised00_output); // Lock the semaphore for output management
            fprintf(file, "%d: U %d: service finished\n", ++(sharedData->action_number), idU);
            fflush(file);
            sem_post(xkised00_output); // Unlock the semaphore for output management        
        }
    }
}

int main(int argc, char* argv[]) {
    // Validate command line arguments
    if (argc != 6) {
        fprintf(stderr, "Usage: %s clerk_count customer_count max_waiting_time max_break_time max_closure_time\n", argv[0]);
    return 1;
    }

    // Parse command line arguments
    int clerk_count = atoi(argv[1]);
    int customer_count = atoi(argv[2]);
    int max_waiting_time = atoi(argv[3]);
    int max_break_time = atoi(argv[4]);
    int max_closure_time = atoi(argv[5]);

    // Validate input values
    if (
        validateValue(max_waiting_time, "max_waiting_time", 1, 10000) ||
        validateValue(max_break_time, "max_break_time", 1, 100) ||
        validateValue(max_closure_time, "max_closure_time", 1, 10000)) {
        return 1;
    }

    // Initialize shared memory
    key_t shm_key = ftok(".", 'x');
    int shm_id = shmget(shm_key, sizeof(SharedData), IPC_CREAT | 0666);
    if (shm_id == -1) {
        fprintf(stderr, "Failed to create shared memory segment\n");
        return 1;
    }
    SharedData* shared_data = (SharedData*)shmat(shm_id, NULL, 0);
    if (shared_data == (void*)-1) {
        fprintf(stderr, "Failed to attach shared memory segment\n");
        return 1;
    }

    srand(time(NULL) ^ (getpid()<<16));

    // Initialize the shared data
    shared_data->clerk_count = clerk_count;
    shared_data->customer_count = customer_count;
    shared_data->max_waiting_time = max_waiting_time;
    shared_data->max_break_time = max_break_time;
    shared_data->max_closure_time = max_closure_time;
    shared_data->post_opened = true;
    shared_data->action_number = 0;
    shared_data->service1 = 0;
    shared_data->service2 = 0;
    shared_data->service3 = 0;

    // Initialize the semaphores
    initialize_semaphores();

    pid_t pid;

    FILE* file = fopen("proj2.out", "w");
    if (file == NULL) {
        fprintf(stderr,"Error opening the file.\n");
        return 1;
    }

    // Create the clerk processes
    for (int i = 0; i < clerk_count; i++) {
        pid = fork();
        if (pid == 0) {
            // Child process
            clerk_process(i + 1, shared_data, file);
            exit(0);
        } else if (pid < 0) {
            // Forking error
            fprintf(stderr,"Error forking clerk process.\n");
            return 1;
        }
    }

    // Create the customer processes
    for (int i = 0; i < customer_count; i++) {
        pid = fork();
        if (pid == 0) {
            // Child process
            customer_process(i + 1, shared_data, file);
            exit(0);
        } else if (pid < 0) {
            // Forking error
            fprintf(stderr,"Error forking customer process.\n");
            return 1;
        }
    }

    int waiting_time = rand() % (max_closure_time/2);
    waiting_time = waiting_time + max_closure_time/2;
    usleep(waiting_time*1000);

    sem_wait(xkised00_output); // Lock the semaphore for output management
    fprintf(file, "%d: closing \n", ++(shared_data->action_number));
    fflush(file);
    sem_wait(xkised00_closure);
    shared_data->post_opened = false;
    sem_post(xkised00_closure);
    sem_post(xkised00_output); // Unlock the semaphore for output management      

    while (wait(NULL)>0);

    // Detach and remove the shared memory segment
    if (shmdt(shared_data) == -1) {
        fprintf(stderr, "Failed to detach shared memory segment\n");
    }
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        fprintf(stderr, "Failed to remove shared memory segment\n");
    }


    // Remove the semaphores

    cleanup();

    return 0;
}
