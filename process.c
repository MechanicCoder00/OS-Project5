#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <semaphore.h>
#define SHMKEY 947532841                    //custom key for shared memory
#define PROBOFTERM 5                        //probability of a the child process terminating
#define NUMOFRD 20                          //number of resource descriptors
#define ACTIVEPROCESSLIMIT 18               //limit for total child processes allowed at once
#define SIZEOFSEM sizeof(sem_t)             //Sizes of shared memory segments
#define SIZEOFCLOCKSIM sizeof(int)*6
#define RDSIZE sizeof(rd)*NUMOFRD
#define RDSIZE2 sizeof(int)*ACTIVEPROCESSLIMIT
#define RDSIZE3 sizeof(int)*ACTIVEPROCESSLIMIT
#define LOGFILELINELIMIT 100000


sem_t *mutex0;                  //shared memory pointers to semaphores
sem_t *mutex1;
sem_t *mutex2;
int *clockSim;                  //shared memory pointer to clock simulator array
int* clockSec;                  //Clock simulator pointer to seconds
int* clockNano;                 //Clock simulator pointer to nanoseconds
int* changesMade;
int* terminationInProgress;
int* verbose;
int* logFileLineCount;
FILE *output;
char outputFileName[12] = "logfile.log";
unsigned int seed = 1;
int *processTable;
int *processTerminationTable;
int terminateFlag = 0;
int bound;

typedef struct          //struct that hold resource descriptors
{
    int allocation;
    int request;
    int available;
    int total;
    int sharable;
    int arrayOfProcessAllocation[ACTIVEPROCESSLIMIT];
    int arrayOfProcessRequests[ACTIVEPROCESSLIMIT];
} rd;
rd *arrayOfRD;


void printClock()   //print clock to output
{
    fprintf(output, " %u:%u\n",*clockSec,*clockNano);
}

unsigned int getRandomNumber(unsigned int min, unsigned int max)       //function to return a random number
{
    srand(time(0)+(seed++) * (unsigned int)getpid());
    unsigned int randomNumber = (unsigned)(min + (rand() % ((max - min) +1 )));
    return randomNumber;
}

int checkIfTerminate()          //will decide if process will be terminating based on probability of termination definition
{
    int randomNumber = getRandomNumber(1,100);
    if(randomNumber > 0 && randomNumber <= PROBOFTERM)
    {
        return 1;
    } else
    {
        return 0;
    }
}

int findElement(int pid)        //function to find element number in process table using pid
{
    int i;
    for(i=0;i<ACTIVEPROCESSLIMIT;i++)
    {
        if(processTable[i] == pid)
        {
            return i;
        }
    }
    return -1;
}

void childCleanup()                     //function to detach child from shared memory
{
    shmdt(mutex0);                                                       //detach from shared memory
    shmdt(mutex1);                                                       //detach from shared memory
    shmdt(mutex2);                                                       //detach from shared memory
    shmdt(clockSim);                                                  //detach from shared memory
    shmdt(arrayOfRD);                                               //detach from shared memory
    shmdt(processTable);
    shmdt(processTerminationTable);
}

void incrementClock(unsigned int secIncrement, unsigned int nanoIncrement)         //function to increment clock simulator
{
    *clockSec = (unsigned int)*clockSec + secIncrement;
    *clockNano = (unsigned int)*clockNano + nanoIncrement;                        //increments clock simulator.

    while((unsigned int)*clockNano >= 1000000000)                      //if nano seconds are equal or greater than 1 second
    {
        *clockSec = (unsigned int)*clockSec + 1;                        //increase seconds by 1
        *clockNano = (unsigned int)*clockNano - 1000000000;               //decrease nano seconds by 1 second
    }
}

void handle_sigterm()                    //signal handler for terminate signal
{
    terminateFlag = 1;
}

void terminate()
{
//    printf("Child: Process %d aborted by Parent\n", getpid());

    if(*verbose == 1 && *logFileLineCount < LOGFILELINELIMIT)
    {
        sem_wait(mutex0);
        output = fopen(outputFileName, "a");
        fprintf(output, "Process P%d terminating by master at time", findElement(getpid()));
        printClock();
        *logFileLineCount += 1;
        fclose(output);
        incrementClock(0,getRandomNumber(10000,500000));
        sem_post(mutex0);
    }else
    {
        sem_wait(mutex0);
        incrementClock(0,getRandomNumber(10000,500000));
        sem_post(mutex0);
    }

    sem_wait(mutex2);
    processTerminationTable[findElement(getpid())] = 1;
    sem_post(mutex2);

    childCleanup();                     //function call to clean up child shared memory
    exit(1);
}

int getValidRequest()       //function that evaluates each resource to make sure there is a resource available then randomly picks one and returns it
{
    int r;
    int validArray[NUMOFRD] = {0};
    int resoucesValidCount = 0;

    for(r=0;r<NUMOFRD;r++)
    {
        if(arrayOfRD[r].available > 0)
        {
            validArray[resoucesValidCount] = r;
            resoucesValidCount++;
        }
    }
    if(resoucesValidCount > 0)
    {
        r = validArray[getRandomNumber(0,resoucesValidCount-1)];
    }
    else
    {
        r = -1;
    }
    return r;
}

void requestR()         //function to  request a random resource with a random number of that resources
{
    int pid = getpid();
    int r = getValidRequest();

    if(r >= 0)
    {
        int totalR = arrayOfRD[r].total;
        int allocatedR = arrayOfRD[r].arrayOfProcessAllocation[findElement(pid)];
        int n = totalR - allocatedR;

        if (n > 0)
        {
            int amountToRequest = getRandomNumber(1, n);
            sem_wait(mutex1);
            if (terminateFlag != 1)
            {
                arrayOfRD[r].request += amountToRequest;
                arrayOfRD[r].arrayOfProcessRequests[findElement(pid)] = amountToRequest;
            }

            sem_post(mutex1);

            if(*verbose == 1 && *logFileLineCount < LOGFILELINELIMIT)
            {
                sem_wait(mutex0);
//                printf("Child %d requesting %d of Resource %d\n", pid, amountToRequest, r);
                output = fopen(outputFileName, "a");
                fprintf(output, "Process P%d requesting %d of R%d at time", findElement(pid), amountToRequest, r);
                printClock();
                *logFileLineCount += 1;
                fclose(output);
                incrementClock(0,getRandomNumber(10000,500000));
                *changesMade = 1;
                sem_post(mutex0);
            } else
            {
                sem_wait(mutex0);
                incrementClock(0,getRandomNumber(10000,500000));
                *changesMade = 1;
                sem_post(mutex0);
            }
            while (arrayOfRD[r].arrayOfProcessRequests[findElement(pid)] != 0 && terminateFlag == 0);    //Child waits until resources are allocated
        }
    }
    if(terminateFlag == 1)
    {
        terminate();
    }
}

int findrandomRtoRelease(int pid)       //function to find a random resource to release and return it
{
    int process = findElement(pid);
    int r;
    int resourcesAllocatedArray[NUMOFRD] = {0};
    int resoucesAllocatedCount = 0;
    for(r=0;r<NUMOFRD;r++)
    {
        if(arrayOfRD[r].arrayOfProcessAllocation[process] > 0)
        {
            resourcesAllocatedArray[resoucesAllocatedCount] = r;
            resoucesAllocatedCount++;
        }
    }
    if(resoucesAllocatedCount > 0)
    {
        r = resourcesAllocatedArray[getRandomNumber(0,resoucesAllocatedCount-1)];
    }
    else
    {
        r = -1;
    }
    return r;
}

void releaseR()     //function to request a release of a random resource
{
    int pid = getpid();
    int r = findrandomRtoRelease(pid);

    if(r == -1)
    {
        return;
    }

    int n = arrayOfRD[r].arrayOfProcessAllocation[findElement(pid)];

    if(n > 0)
    {
        int amountToRequest = getRandomNumber(1,n);

        sem_wait(mutex1);
        if(terminateFlag != 1)
        {
            arrayOfRD[r].request -= amountToRequest;
            arrayOfRD[r].arrayOfProcessRequests[findElement(pid)] = -(amountToRequest);

            if(*verbose == 1 && *logFileLineCount < LOGFILELINELIMIT)
            {
                sem_wait(mutex0);
//            printf("Child %d releasing %d of Resource %d\n",pid,-(amountToRequest),r);
                output = fopen(outputFileName, "a");
                fprintf(output, "Process P%d releasing %d of R%d at time", findElement(pid), amountToRequest, r);
                printClock();
                *logFileLineCount += 1;
                fclose(output);
                incrementClock(0,getRandomNumber(10000,500000));
                *changesMade = 1;
                sem_post(mutex0);
            }else
            {
                sem_wait(mutex0);
                incrementClock(0,getRandomNumber(10000,500000));
                *changesMade = 1;
                sem_post(mutex0);
            }
        }
        sem_post(mutex1);

        while(arrayOfRD[r].arrayOfProcessRequests[findElement(pid)] != 0 && terminateFlag == 0);    //Child waits until resources are allocated
    }
    if(terminateFlag == 1)
    {
        terminate();
    }
}

void requestTermination()   //function to tell master that it is terminating by setting a flag in the termination table
{
    if(*verbose == 1 && *logFileLineCount < LOGFILELINELIMIT)
    {
        sem_wait(mutex0);
        output = fopen(outputFileName, "a");
        fprintf(output, "Process P%d terminating successfully at time", findElement(getpid()));
        printClock();
        *logFileLineCount += 1;
        fclose(output);
        sem_post(mutex0);
    }

    sem_wait(mutex2);
    if(processTerminationTable[findElement(getpid())] != 1)
    {
        *terminationInProgress += 1;
        processTerminationTable[findElement(getpid())] = 1;
    }
    sem_post(mutex2);
}

void sharedMemorySetup()    //function to setup shared memory
{
    int shmid0 = shmget(SHMKEY, SIZEOFSEM, 0600 | IPC_CREAT);       //creates shared memory id for the semaphore
    if (shmid0 == -1)
    {
        perror("Shared memory0C:");                         //if shared memory does not exist print error message and exit
        exit(1);
    }
    mutex0 = (sem_t*)shmat(shmid0,NULL,0);                   //attaches semaphore to shared memory segment
    if (mutex0 == (sem_t*)-1)
    {
        perror("Mutex shmat 0C:");                          //if shared memory does not exist print error message and exit
        exit(1);
    }

    int shmid1 = shmget(SHMKEY+1, SIZEOFSEM, 0600 | IPC_CREAT);       //creates shared memory id for the semaphore
    if (shmid1 == -1)
    {
        perror("Shared memory1C:");                         //if shared memory does not exist print error message and exit
        exit(1);
    }
    mutex1 = (sem_t*)shmat(shmid1,NULL,0);                   //attaches semaphore to shared memory segment
    if (mutex1 == (sem_t*)-1)
    {
        perror("Mutex shmat 1C:");                          //if shared memory does not exist print error message and exit
        exit(1);
    }

    int shmid2 = shmget (SHMKEY+2, SIZEOFCLOCKSIM, 0600 | IPC_CREAT);     //creates shared memory id for the fixed size array
    if (shmid2 == -1)
    {
        perror("Shared memory2C:");                                   //if shared memory does not exist print error message and exit
        exit(1);
    }
    clockSim = (int *)shmat(shmid2, 0, 0);                  //attaches to the fixed shared memory array
    clockSec = &clockSim[0];                                //creates a pointer to the clock seconds
    clockNano = &clockSim[1];                               //creates a pointer to the clock nanoseconds
    terminationInProgress = &clockSim[2];                   //creates a pointer to a variable to keep track of if a termination is in progress
    changesMade = &clockSim[3];                             //creates a pointer to keep track of changes to resource descriptors in order to decide if run deadlock detection needs to run again
    verbose = &clockSim[4];                                 //creates a pointer to keep track of the verbose setting
    logFileLineCount = &clockSim[5];                        //creates a pointer to keep track of how many lines are in the log file

    int shmid3 = shmget(SHMKEY+3, RDSIZE, 0600 | IPC_CREAT);    //creates shared memory id for the variable size array
    if (shmid3 == -1)
    {
        perror("Shared memory3C:");                                  //if shared memory does not exist print error message and exit
        exit(1);
    }
    arrayOfRD = (rd *)shmat(shmid3, 0, 0);

    int shmid4 = shmget(SHMKEY+4, RDSIZE2, 0600 | IPC_CREAT);    //creates shared memory id for the variable size array
    if (shmid4 == -1)
    {
        perror("Shared memory4C:");                                  //if shared memory does not exist print error message and exit
        exit(1);
    }
    processTable = (int*)shmat(shmid4, 0, 0);

    int shmid5 = shmget(SHMKEY+5, RDSIZE3, 0600 | IPC_CREAT);    //creates shared memory id for the variable size array
    if (shmid5 == -1)
    {
        perror("Shared memory5C:");                                  //if shared memory does not exist print error message and exit
        exit(1);
    }
    processTerminationTable = (int*)shmat(shmid5, 0, 0);

    int shmid6 = shmget(SHMKEY+6, SIZEOFSEM, 0600 | IPC_CREAT);       //creates shared memory id for the semaphore
    if (shmid6 == -1)
    {
        perror("Shared memory6C:");                         //if shared memory does not exist print error message and exit
        exit(1);
    }
    mutex2 = (sem_t*)shmat(shmid6,NULL,0);                   //attaches semaphore to shared memory segment
    if (mutex2 == (sem_t*)-1)
    {
        perror("Mutex shmat 6C:");                          //if shared memory does not exist print error message and exit
        exit(1);
    }
}


int main (int argc, char *argv[])
{
    bound = getRandomNumber(1,3);           //randomly sets the bound which is used to determine if a process requests something
    signal(SIGTERM, handle_sigterm);        //initialization of termination signal handler
    sharedMemorySetup();
    int term = 0;
    while(term != 1 && terminateFlag != 1)
    {
        if(bound == getRandomNumber(1,3))
        {
            sem_wait(mutex0);
            incrementClock(0,getRandomNumber(10000,500000));
            sem_post(mutex0);

            int requestOrRelease = getRandomNumber(0,1);    //determines if request is requesting or releasing a resource 0-request  1-release
            if(requestOrRelease == 0)
            {
                requestR();
            } else
            {
                releaseR();
            }
        }
        term = checkIfTerminate();  //set terminate variable to exit the process loop and terminate
    }
    if(terminateFlag == 0)
    {
        requestTermination();
    }

    sem_wait(mutex0);
    incrementClock(0,getRandomNumber(10000,500000));
    sem_post(mutex0);

    childCleanup();     //function call to clean up child shared memory

    return 0;
}