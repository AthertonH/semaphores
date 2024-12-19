#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <errno.h> // for errno
#include <string.h>
#include <ctype.h>

const unsigned int VEHICLE_CNT_MIN = 10;
const unsigned int VEHICLE_CNT_MAX = 500;

const unsigned int LIGHT_WAIT = 4;
const unsigned int YELLOW_WAIT = 1;
const unsigned int TRANSIT_TIME_BASE = 1;
const unsigned int ARRIVAL_INTERVAL = 2;

const unsigned int NORTH = 0;
const unsigned int EAST = 1;
const unsigned int SOUTH = 2;
const unsigned int WEST = 3;

typedef struct __Zem_t 
{
    int value;
    pthread_cond_t cond;
    pthread_mutex_t lock;
} Zem_t;

void Zem_init(Zem_t *s, int value)
{
    s->value = value;
    pthread_cond_init(&s->cond, NULL);
    pthread_mutex_init(&s->lock, NULL);
}

void Zem_wait(Zem_t *s)
{
    pthread_mutex_lock(&s->lock);

    while (s->value <= 0)
        pthread_cond_wait(&s->cond, &s->lock);

    s->value--;
    pthread_mutex_unlock(&s->lock); 
}

void Zem_post(Zem_t *s)
{
    pthread_mutex_lock(&s->lock);
    s->value++;
    pthread_cond_signal(&s->cond);
    pthread_mutex_unlock(&s->lock);
}

typedef struct
{
    int direction;
    Zem_t* trafficLights; // Pointer to array
} VehicleArgs;

bool getArguments(int argc, char* argv[], int* vc)
{
    // Usage
    if(argc == 1)
        {printf("Usage: ./traffic -vc <vehicleCount>\n"); return false;}

    if(argc != 3)
        {printf("Error, invalid command line options.\n"); return false;}

    // -v
    if(strcmp(argv[1], "-vc") != 0)
        {printf("Error, invalid item count specifier.\n"); return false;}

    // Thread amount
    for(int i = 0; argv[2][i] != '\0'; i++)
    {
        if(!isdigit(argv[2][i]))
            {printf("Error, invalid vehicle count value.\n"); return false;}  
    }

    // Vehicle count range
    if(atoi(argv[2]) > VEHICLE_CNT_MAX || atoi(argv[2]) < VEHICLE_CNT_MIN)
        {printf("Error, vehicle count out of range.\n"); return false;}

    *vc = atoi(argv[2]);
    return true;
}

void* trafficLightController(void* arg)
{
    Zem_t* trafficLights = (Zem_t*)arg;

    while (1) {
        // North-South green
        printf("\033[0;92mGreen light for North-South\033[0m\n");
        Zem_post(&trafficLights[NORTH]);
        Zem_post(&trafficLights[SOUTH]);
        sleep(LIGHT_WAIT);

        // North-South yellow
        printf("\033[0;93mYellow light for North-South\033[0m\n");
        sleep(YELLOW_WAIT);

        // North-South red
        printf("\033[0;31mRed light for North-South\033[0m\n");
        Zem_wait(&trafficLights[NORTH]);
        Zem_wait(&trafficLights[SOUTH]);

        // East-West green
        printf("\033[0;92mGreen light for East-West\033[0m\n");
        Zem_post(&trafficLights[EAST]);
        Zem_post(&trafficLights[WEST]);
        sleep(LIGHT_WAIT);

        // East-West yellow
        printf("\033[0;93mYellow light for East-West\033[0m\n");
        sleep(YELLOW_WAIT);

        // East-West red
        printf("\033[0;31mRed light for East-West\033[0m\n");
        Zem_wait(&trafficLights[EAST]);
        Zem_wait(&trafficLights[WEST]);
    }

    return NULL;
}

void* vehicleThread(void* arg) {
    // Cast argument to the vehicle struct
    VehicleArgs* args = (VehicleArgs*)arg;
    const char* directionString[] = {"North", "East", "South", "West"};

    // Access the direction and sempahore array
    int direction = args->direction;
    Zem_t* trafficLights = args->trafficLights;

    // Print approaching message
    printf("↕ Vehicle approaching from %s.\n", directionString[direction]);

    // Wait for light to be green (via sempahore)
    Zem_wait(&trafficLights[direction]);

    // Print passing through message (including direction)
    printf("↔ Vehicle passing through from %s.\n", directionString[direction]);

    // Wait for vehicle to pass through the intersection
    sleep(TRANSIT_TIME_BASE);
    usleep(rand() % 5000);

    // Release, light turns red (via semaphore)
    Zem_post(&trafficLights[direction]);

    return NULL;  // End thread
}

int main(int argc, char *argv[])
{
    int vehicleCount = 0;

    // Error check arguments
    getArguments(argc, argv, &vehicleCount);

    printf("\033[0;4mCS 370 Project #5B -> Traffic Light Simulation Project\033[0m\n");
    printf("Vehicles: %d\n\n", vehicleCount);

    // Dyanmically initialize semaphores (trafficLights)
    Zem_t* trafficLights = malloc(4 * sizeof(Zem_t));

    // Initially set all semaphores/lights to red (0)
    for(int i = 0; i < 4; i++)
        Zem_init(&trafficLights[i], 0);

    // Create traffic light controller thread
    pthread_t controllerThread;
    pthread_create(&controllerThread, NULL, trafficLightController, (void*)trafficLights);

    // Create array for vehicle threads
    pthread_t* vehicleThreads = malloc(vehicleCount * sizeof(pthread_t));

    // Create vehicle threads
    for(int i = 0; i < vehicleCount; i++)
    {
        // Allocate memory for the struct
        VehicleArgs* args = malloc(sizeof(VehicleArgs));
        args->direction = rand() % 4;
        args->trafficLights = trafficLights;

        // Create the thread
        pthread_create(&vehicleThreads[i], NULL, vehicleThread, (void*)args);

        // Vehicle arrival time
        sleep(ARRIVAL_INTERVAL);
    }

    // Join to wait for all vehicles to pass
    for(int i = 0; i < vehicleCount; i++)
        pthread_join(vehicleThreads[i], NULL);

    pthread_cancel(controllerThread);
    pthread_join(controllerThread, NULL);

    free(vehicleThreads);
    for(int i = 0; i < 4; i++)
    {
        pthread_cond_destroy(&trafficLights[i].cond);
        pthread_mutex_destroy(&trafficLights[i].lock);
    }

    free(trafficLights);

    printf("\nAll vehicles successfully passed through the intersection.\n");
}