#include "hw6.h"
//#include "util.h" // <---- for MAC_OS (pthread_barrier_t) NEED WHEN RUNNING ON MY MAC 
#include <stdio.h>
#include <pthread.h>
/*****SOURCES********
https://blog.albertarmea.com/post/47089939939/using-pthreadbarrier-on-mac-os-x  --> this link is for the util.h because MACOS doesnt have the pthread library
Man document pthread 
https://linux.die.net/man/3/pthread_barrier_init 
lab 11 : https://piazza.com/class_profile/get_resource/jzis3awdbyu7e8/k3eiu9xmmyy6ag
lab 12 : https://piazza.com/class_profile/get_resource/jzis3awdbyu7e8/k3nud9vo2rt3bk
*/

//created struct of elevators for bookkeeping info
struct multi_elevator
{
    int dest_floor; //destination floor 
    int pick_passenger; //passenger thats choosen 
    int direction; //direction elevator needs to go 
    int from_floor; // current position 
    int occupancy; //number of passengers in the elevator--> should be one at a time 

    //GOOGLED:
    //NOTE: a mutual exclusion object (mutex) is a program object that allows 
    //multiple program threads to share the same resource, such as file access, 
    //but not simultaneously. When a program is started, a mutex is created with a unique name.

    //NOTE: using barriers in pthreads. pthreads allows us to create and manage threads 
    //in a program. When multiple threads are working together, it might be required that 
    //the threads wait for each other at a certain event or point in the program before proceeding ahead.

    pthread_mutex_t elevatorz;
    pthread_barrier_t elevBarrier;
};
struct multi_elevator M_ELEV[ELEVATORS];

enum
{
    ELEVATOR_ARRIVED = 1, ELEVATOR_OPEN = 2,ELEVATOR_CLOSED = 3
} state; //never used 

pthread_barrier_t passengerBarriers[PASSENGERS]; 

int elavator_checker[ELEVATORS];         // array to act as an initial checker
int passenger_request_array[PASSENGERS]; // store passenger requests in array

void scheduler_init()
{ //to allocate resources for a barrier and initialize its attributes.

    //initialize all passenger barriers
    for (int j = 0; j < PASSENGERS; j++){
        pthread_barrier_init(&passengerBarriers[j], 0, 2); //2 is the number of threads that need to be met per barrier.
    }

    //initilize the data structure to -1
    for (int l = 0; l < ELEVATORS; l++){
        elavator_checker[l] = -1;
    }

    //initialize everything in the struct.
    for (int m = 0; m < ELEVATORS; m++){
        M_ELEV[m].dest_floor = -1;
        M_ELEV[m].direction = -1;
        M_ELEV[m].pick_passenger = -1;
        M_ELEV[m].occupancy = 0;
        pthread_mutex_init(&M_ELEV[m].elevatorz, 0);
        pthread_barrier_init(&M_ELEV[m].elevBarrier, 0, 2);
    }
}

void passenger_request(int passenger, int from_floor, int to_floor,
                       void (*enter)(int, int),
                       void (*exit)(int, int))
{

    passenger_request_array[passenger] = to_floor; //store the floor of the passenger request to what floor they wanna go to.

    int theElevator = passenger % ELEVATORS;  //mod so remainder will be id to specific elevator to person 

    pthread_mutex_lock(&M_ELEV[theElevator].elevatorz); //which ever elevator gets assigend, it will now be placed as part of the struct and will allow multiple program threads.
    
    M_ELEV[theElevator].occupancy++; //add a passenger onto the elevator.

    pthread_mutex_unlock(&M_ELEV[theElevator].elevatorz); //unlock the elevator

    //********

    pthread_mutex_lock(&M_ELEV[theElevator].elevatorz); //lock 

    M_ELEV[theElevator].pick_passenger = passenger; //pick the current passenger
    
    M_ELEV[theElevator].from_floor = from_floor; //get the current location 
    M_ELEV[theElevator].dest_floor = to_floor;   //get the location the pssenger wants to go to 

    //so in the struct every elevator has a barrier to know when to do what job 

    pthread_barrier_wait(&M_ELEV[theElevator].elevBarrier);

    pthread_barrier_wait(&passengerBarriers[passenger]); // wait for elevator to tell you get on
    enter(passenger, theElevator); //is this 0 the elevator??? --> now that the struct is created you need to use theElevator to as the other parameter cause its the one thats assigned.
    pthread_barrier_wait(&passengerBarriers[passenger]); // tell elevator you entered

    pthread_barrier_wait(&passengerBarriers[passenger]); // wait for elevator to tell you to get off
    exit(passenger, theElevator);
    pthread_barrier_wait(&passengerBarriers[passenger]); // tell elevator you exit

    pthread_mutex_unlock(&M_ELEV[theElevator].elevatorz); //unlock
}

void elevator_ready(int elevator, int at_floor,
                    void (*move_direction)(int, int),
                    void (*door_open)(int), void (*door_close)(int))
{

    if (elavator_checker[elevator] == -1){
        elavator_checker[elevator] = 0;
    }
     
    //if there are 0 are less passengers than just exit cause no ones there.
    //with out this the program will just continue to run even if no passegngers are there 
    if(M_ELEV[elevator].occupancy <= 0){   
        pthread_exit(NULL);
    }

    pthread_barrier_wait(&M_ELEV[elevator].elevBarrier);
     
    move_direction(elevator, M_ELEV[elevator].from_floor - at_floor); //moving around in the direction that the passenger calls from after pushing the button and the elevator has to head there.

    door_open(elevator);
    pthread_barrier_wait(&passengerBarriers[M_ELEV[elevator].pick_passenger]); //let passenger know to come inside

    pthread_barrier_wait(&passengerBarriers[M_ELEV[elevator].pick_passenger]); // wait for passenger to tell you he/she got on
    door_close(elevator);
    //pthread_barrier_wait(&passengerBarriers[M_ELEV[elevator].pick_passenger]); 


    move_direction(elevator, M_ELEV[elevator].dest_floor - M_ELEV[elevator].from_floor);//moving around in the direction you need to go to drop off the passenger

    door_open(elevator);
    pthread_barrier_wait(&passengerBarriers[M_ELEV[elevator].pick_passenger]); //let passenger know to they can leave 

    pthread_barrier_wait(&passengerBarriers[M_ELEV[elevator].pick_passenger]); // wait for passenger to tell you he got off
    door_close(elevator);
    //pthread_barrier_wait(&passengerBarriers[M_ELEV[elevator].pick_passenger]);

    M_ELEV[elevator].occupancy --; //decrement num of passengers in the elevator. 

}
