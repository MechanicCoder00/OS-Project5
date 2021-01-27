#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#define SHMKEY 947532841                    //custom key for shared memory
#define NUMOFRD 20                          //number of resource descriptors
#define ACTIVEPROCESSLIMIT 18               //limit for total child processes allowed at once
#define SIZEOFSEM sizeof(sem_t)             //Sizes of shared memory segments
#define SIZEOFCLOCKSIM sizeof(int)*6
#define RDSIZE sizeof(rd)*NUMOFRD
#define RDSIZE2 sizeof(int)*ACTIVEPROCESSLIMIT
#define RDSIZE3 sizeof(int)*ACTIVEPROCESSLIMIT
#define TOTALPROCESSESLIMIT 150         //Limit for the total number of processes
#define MINTIMEBETWEENNEWPROCSNS 5000000
#define MAXTIMEBETWEENNEWPROCSNS 1500000000
#define LOGFILELINELIMIT 100000

/*
 * Project : Operating systems assignment 5
 * Author : Scott Tabaka
 * Date Due : 4/15/2020
 * Course : CMPSCI 4760
 * Purpose : This program simulates operating system resource management by allocating/deallocating resources, checks
 *          for deadlocks using a deadlock detection algorithm and resolves the deadlock by terminating the process
 *          with the most total allocated resources.
 */


int shmid0;                 //variables to store id for shared memory segments
int shmid1;
int shmid2;
int shmid3;
int shmid4;
int shmid5;
int shmid6;
sem_t *mutex0;              //shared memory pointers to semaphores
sem_t *mutex1;
sem_t *mutex2;
int *clockSim;              //shared memory pointer to array
int* clockSec;              //Pointers to clock simulator
int* clockNano;
int* changesMade;
int* terminationInProgress;
int* verbose;
int* logFileLineCount;
int *processTable;
int *processTerminationTable;
FILE *output;
char outputFileName[12] = "logfile.log";
unsigned int seed = 1;      //helper number to create random numbers, increases each time it is used
int activeChildren = 0;     //variable to keep track of active children
int opt;                    //variable for checking options
static char usage[100];     //array for storing a usage message
static char error[100];     //array for storing a error message
int v = 0;
int sharableProb;
unsigned int nextProcessLaunchTimeSec;
unsigned int nextProcessLaunchTimeNSec;
int totalProcessesLaunched = 0;
int currentPid;
int currentResource;
int processesInTable = 0;
int deadlockProcessArray[ACTIVEPROCESSLIMIT] = {1};     // 0-already processed    1-needs to be processed
int deadlockRAvailableArray[NUMOFRD];
int numProcLeftToCheck;
int deadlockProgramStatus = 0;
int deadlockAbort = 0;
int zombiesToClean = 0;
int deadlockedList[ACTIVEPROCESSLIMIT];
int deadlockedCount;
int totalRequestsGranted = 0;
int totalDeadlockedProcessesTerminated = 0;
int totalSucessfulTerminations = 0;
int totalTimesDeadlockDetectionHasRun = 0;
double avgPercentageOfDeadlockedProcessesTerminated = 0;
int totalDeadlockedProcesses = 0;
int requestCount = 0;

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


void printClock()       //print clock to output
{
    fprintf(output, " %u:%u\n",*clockSec,*clockNano);
}

void displayClock()   //debugging clock display
{
    printf("Parent Clock: %u:%u\n",*clockSec,*clockNano);
}

void calculateAvgPercentageOfDeadlockedProcesses()      //function to calculate average percentage of deadlocked processes
{
    avgPercentageOfDeadlockedProcessesTerminated = (double)totalDeadlockedProcessesTerminated/totalDeadlockedProcesses;
    avgPercentageOfDeadlockedProcessesTerminated = avgPercentageOfDeadlockedProcessesTerminated * 100;
}

int findElement(int pid)        //function to find element number using pid
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

void displayRD()    //debug display for resource descriptors
{
    int i;
    printf("Resource Descriptors:\n");
    printf("    Allocation /  Request   / Available  /   Total    /  Sharable  \n");
    for(i=0;i<NUMOFRD;i++)
    {
        fprintf(stdout, "R%-2d:   %-12d %-12d",i,arrayOfRD[i].allocation,arrayOfRD[i].request);
        fprintf(stdout, " %-12d %-12d %-12d\n",arrayOfRD[i].available, arrayOfRD[i].total,arrayOfRD[i].sharable);
    }
}

void displayRDToOutput()    //prints resource descriptors to output
{
    int i;
    fprintf(output,"Resource Descriptors:\n");
    fprintf(output,"    Allocation /  Request   / Available  /   Total    /  Sharable  \n");
    for(i=0;i<NUMOFRD;i++)
    {
        fprintf(output, "R%-2d:   %-12d %-12d",i,arrayOfRD[i].allocation,arrayOfRD[i].request);
        fprintf(output, " %-12d %-12d %-12d\n",arrayOfRD[i].available, arrayOfRD[i].total,arrayOfRD[i].sharable);
    }
}

void displayRDAllocatedTable()  //debug display for allocation table
{
    int i;
    printf("Allocated Table:\n");
    printf("    ");
    for(i=0;i<ACTIVEPROCESSLIMIT;i++)
    {
        fprintf(stdout, "P%-3d", i);
    }
    printf("\n");
    for(i=0;i<NUMOFRD;i++)
    {
        fprintf(stdout, "R%-4d", i);
        int j;
        for(j=0;j<ACTIVEPROCESSLIMIT;j++)
        {
            fprintf(stdout, "%-4d", arrayOfRD[i].arrayOfProcessAllocation[j]);
        }
        printf("\n");
    }
}

void displayRDAllocatedTableToOutput()      //prints allocation table to output
{
    int i;
    fprintf(output,"Allocated Table:\n");
    fprintf(output,"    ");
    for(i=0;i<ACTIVEPROCESSLIMIT;i++)
    {
        fprintf(output, "P%-3d", i);
    }
    fprintf(output,"\n");
    for(i=0;i<NUMOFRD;i++)
    {
        fprintf(output, "R%-4d", i);
        int j;
        for(j=0;j<ACTIVEPROCESSLIMIT;j++)
        {
            fprintf(output, "%-4d", arrayOfRD[i].arrayOfProcessAllocation[j]);
        }
        fprintf(output,"\n");
    }
}

void displayRDRequestTable()        //debug display for request table
{
    int i;
    printf("Requested Table:\n");
    printf("    ");
    for(i=0;i<ACTIVEPROCESSLIMIT;i++)
    {
        fprintf(stdout, "P%-3d", i);
    }
    printf("\n");
    for(i=0;i<NUMOFRD;i++)
    {
        fprintf(stdout, "R%-4d", i);
        int j;
        for(j=0;j<ACTIVEPROCESSLIMIT;j++)
        {
            fprintf(stdout, "%-4d", arrayOfRD[i].arrayOfProcessRequests[j]);
        }
        printf("\n");
    }
}

void displayRDRequestTableToOutput()        //prints request table to output
{
    int i;
    fprintf(output,"Requested Table:\n");
    fprintf(output,"    ");
    for(i=0;i<ACTIVEPROCESSLIMIT;i++)
    {
        fprintf(output, "P%-3d", i);
    }
    fprintf(output,"\n");
    for(i=0;i<NUMOFRD;i++)
    {
        fprintf(output, "R%-4d", i);
        int j;
        for(j=0;j<ACTIVEPROCESSLIMIT;j++)
        {
            fprintf(output, "%-4d", arrayOfRD[i].arrayOfProcessRequests[j]);
        }
        fprintf(output,"\n");
    }
}

void displayProcessTable()      //debug display for process table
{
    int i;
    printf("              ");
    for(i=0;i<ACTIVEPROCESSLIMIT;i++)
    {
        fprintf(stdout, "P%-5d", i);
    }
    printf("\n");
    printf("Process Table:");
    for(i=0;i<ACTIVEPROCESSLIMIT;i++)
    {
        fprintf(stdout, "%-6d", processTable[i]);
    }
    printf("\n");
}

void displayProcessTerminationTable()       //debug display for termination table
{
    int i;
    printf("Termination:  ");
    for(i=0;i<ACTIVEPROCESSLIMIT;i++)
    {
        fprintf(stdout, "%-6d", processTerminationTable[i]);
    }
    printf("\n");
}

void displayStats()     //prints stats to output
{
    calculateAvgPercentageOfDeadlockedProcesses();

    fprintf(output,"Total Sucessful Terminations:%d\n",totalSucessfulTerminations);
    fprintf(output,"Total Deadlocked Processes Terminated:%d\n",totalDeadlockedProcessesTerminated);
    fprintf(output,"Total Deadlocked Processes:%d\n",totalDeadlockedProcesses);
    fprintf(output,"Average Percentage Of Deadlocked Processes Terminated:%.2f percent\n",avgPercentageOfDeadlockedProcessesTerminated);
    fprintf(output,"Total Requests Granted:%d\n",totalRequestsGranted);
    fprintf(output,"Total Times Deadlock Detection Has Run:%d\n",totalTimesDeadlockDetectionHasRun);


//    printf("totalDeadlockedProcesses:%d\n",totalDeadlockedProcesses);
//    printf("totalDeadlockedProcessesTerminated:%d\n",totalDeadlockedProcessesTerminated);
//    printf("totalSucessfulTerminations:%d\n",totalSucessfulTerminations);
//    printf("totalRequestsGranted:%d\n",totalRequestsGranted);
//    printf("totalTimesDeadlockDetectionHasRun:%d\n",totalTimesDeadlockDetectionHasRun);
//    fprintf(stdout,"avgPercentageOfDeadlockedProcessesTerminated:%.2f percent\n",avgPercentageOfDeadlockedProcessesTerminated);
}

void displayAll()       //function to print or display everything
{
    sem_wait(mutex0);
    output = fopen(outputFileName, "a");

//    displayClock();
//    displayRD();
//    printf("\n");
//    displayRDAllocatedTable();
//    printf("\n");
//    displayRDRequestTable();
//    printf("\n");
//    displayProcessTable();
//    displayProcessTerminationTable();

    fprintf(output,"\n");
    fprintf(output,"Program End Time: ");
    printClock();
    fprintf(output,"\n");
    displayRDToOutput();
    fprintf(output,"\n");
    displayRDAllocatedTableToOutput();
    fprintf(output,"\n");
    displayRDRequestTableToOutput();
    fprintf(output,"\n");
    displayStats();

    fclose(output);
    sem_post(mutex0);

//    printf("Total Processes Launched:%d  Active Children:%d\n",totalProcessesLaunched,activeChildren);
//    printf("\n*************************************************************************************************\n\n");
//    **********Size of shared memory segments************
//    printf("Size of Sem:%llu\n",SIZEOFSEM);
//    printf("Size of ClockSim:%llu\n",SIZEOFCLOCKSIM);
//    printf("Size of RD:%llu\n",RDSIZE);
//    printf("Size of RD2:%llu\n",RDSIZE2);
//    printf("Size of RD3:%llu\n",RDSIZE3);
}

unsigned int getRandomNumber(unsigned int min, unsigned int max)       //function to return a random number
{
    srand(time(0)+seed++);
    unsigned int randomNumber = (unsigned)(min + (rand() % ((max - min) +1 )));
    return randomNumber;
}

void getNextProcessTime()       //calculates a time in the future to generate a new process
{
    unsigned int nextSec;
    unsigned int nextNSec = getRandomNumber(MINTIMEBETWEENNEWPROCSNS,MAXTIMEBETWEENNEWPROCSNS);
    unsigned int currentSec = clockSim[0];
    unsigned int currentNSec = clockSim[1];

    nextProcessLaunchTimeSec = currentSec;
    nextProcessLaunchTimeNSec = (unsigned)currentNSec + nextNSec;

    while((unsigned int)nextProcessLaunchTimeNSec >= 1000000000)                      //if nano seconds are equal or greater than 1 second
    {
        nextProcessLaunchTimeSec = (unsigned int)nextProcessLaunchTimeSec + 1;                        //increase seconds by 1
        nextProcessLaunchTimeNSec = (unsigned int)nextProcessLaunchTimeNSec - 1000000000;               //decrease nano seconds by 1 second
    }
}

int getSharableClass()      //function that uses the sharedProb var(15-20%) and determines a shareable value and returns it  -- 0=not sharable  1=shareable
{
    int sharablenum = getRandomNumber(1,100);
    if(sharablenum <= sharableProb)
    {
        return 1;
    } else
    {
        return 0;
    }
}

void printHelp()            //function to print help info
{
    printf("%s\n\n", usage);
    printf("This program simulates operating system resource management\n\n");
    printf("-h Print a help message and exit.\n");
    printf("-v Verbose On: Increases the amount of output sent to output file.\n");

    exit(0);
}

void parentCleanup()            //function to cleanup shared memory and semaphores
{
    sem_destroy(mutex0);                                                 //remove the semaphore
    shmdt(mutex0);                                                       //detach from shared memory
    sem_destroy(mutex1);                                                 //remove the semaphore
    shmdt(mutex1);                                                       //detach from shared memory
    sem_destroy(mutex2);                                                 //remove the semaphore
    shmdt(mutex2);                                                       //detach from shared memory
    shmdt(clockSim);                                                     //detach from shared memory
    shmdt(arrayOfRD);
    shmdt(processTable);
    shmdt(processTerminationTable);
    shmctl(shmid0,IPC_RMID,NULL);                                       //removes shared memory segments
    shmctl(shmid1,IPC_RMID,NULL);
    shmctl(shmid2,IPC_RMID,NULL);
    shmctl(shmid3,IPC_RMID,NULL);
    shmctl(shmid4,IPC_RMID,NULL);
    shmctl(shmid5,IPC_RMID,NULL);
    shmctl(shmid6,IPC_RMID,NULL);
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

void initializeMessages(char* str)      //function to set text for usage and error messages
{
    strcpy(error,str);
    strcat(error,": Error:");

    strcpy(usage,"Usage: ");
    strcat(usage,str);
    strcat(usage," [-h] | [-v]");
}

void allocateR(int pid, int r, int n)       //function to allocate a request
{
    if(arrayOfRD[r].available >= n)
    {
        if(arrayOfRD[r].sharable == 1 || (arrayOfRD[r].sharable == 0 && arrayOfRD[r].available == arrayOfRD[r].total))
        {
            int process = findElement(pid);
            if (process < 0)
            {
                fprintf(stderr, "%s allocateR error\n", error);
                exit(1);
            }
            arrayOfRD[r].arrayOfProcessAllocation[process] += n;
            arrayOfRD[r].allocation += n;
            arrayOfRD[r].available -= n;
            sem_wait(mutex1);
            arrayOfRD[r].request -= n;
            sem_post(mutex1);
            arrayOfRD[r].arrayOfProcessRequests[process] -= n;
            totalRequestsGranted++;
            requestCount++;
            if(requestCount == 20)
            {
                if(*verbose == 1 && *logFileLineCount < LOGFILELINELIMIT)
                {
                    sem_wait(mutex0);
                    output = fopen(outputFileName, "a");
                    fprintf(output,"\n");
                    displayRDToOutput();
                    fprintf(output,"\n");
                    displayRDAllocatedTableToOutput();
                    fprintf(output,"\n");
                    displayRDRequestTableToOutput();
                    fprintf(output,"\n");
                    *logFileLineCount += 70;
                    fclose(output);
                    sem_post(mutex0);
                }
                requestCount = 0;
            }
            if(*verbose == 1 && *logFileLineCount < LOGFILELINELIMIT)
            {
                sem_wait(mutex0);
//                printf("Parent: Allocating request for PID:%d: %d of resource %d\n", pid, n, r);
                output = fopen(outputFileName, "a");
                fprintf(output, "Master allocating request to P%d: %d of R%d", findElement(pid), n, r);
                printClock();
                *logFileLineCount += 1;
                fclose(output);
                incrementClock(1,getRandomNumber(1000,500000));
                *changesMade = 1;
                sem_post(mutex0);

            }else
            {
                sem_wait(mutex0);
                incrementClock(1,getRandomNumber(1000,500000));
                sem_post(mutex0);
            }
        }
    }
}

void deallocateR(int process, int r, int n)                //function to deallocate resources   -- if n=0 then it will clear everything
{
        if(n == 0)
        {
            if(arrayOfRD[r].arrayOfProcessAllocation[process] > 0)
            {
                n = arrayOfRD[r].arrayOfProcessAllocation[process];
            }
        }else if(n < 0)
        {
            if(arrayOfRD[r].arrayOfProcessRequests[process] < 0)
            {
                n = abs(n);

                sem_wait(mutex1);
                arrayOfRD[r].request += n;
                sem_post(mutex1);

                arrayOfRD[r].arrayOfProcessAllocation[process] -= n;
                arrayOfRD[r].allocation -= n;
                arrayOfRD[r].available += n;
                arrayOfRD[r].arrayOfProcessRequests[process] += n;
                totalRequestsGranted++;
                requestCount++;
                if(requestCount == 20)
                {
                    if(*verbose == 1 && *logFileLineCount < LOGFILELINELIMIT)
                    {
                        sem_wait(mutex0);
                        output = fopen(outputFileName, "a");
                        fprintf(output,"\n");
                        displayRDToOutput();
                        fprintf(output,"\n");
                        displayRDAllocatedTableToOutput();
                        fprintf(output,"\n");
                        displayRDRequestTableToOutput();
                        fprintf(output,"\n");
                        *logFileLineCount += 70;
                        fclose(output);
                        sem_post(mutex0);
                    }
                    requestCount = 0;
                }

                if(*verbose == 1 && *logFileLineCount < LOGFILELINELIMIT)
                {
                    sem_wait(mutex0);
//                    printf("Parent: Deallocating %d of r:%d for pid:%d",n,r,processTable[process]);
                    output = fopen(outputFileName, "a");
                    fprintf(output, "Master deallocating request to P%d: %d of R%d", process, n, r);
                    printClock();
                    *logFileLineCount += 1;
                    fclose(output);
                    incrementClock(1,getRandomNumber(1000,500000));
                    sem_post(mutex0);
                }else
                {
                    sem_wait(mutex0);
                    incrementClock(1,getRandomNumber(1000,500000));
                    sem_post(mutex0);
                }
                return;
            }
        }else
        {
            n = 0;
        }
        arrayOfRD[r].arrayOfProcessAllocation[process] -= n;
        arrayOfRD[r].allocation -= n;
        arrayOfRD[r].available += n;
}

void loadProcessIntoTable(int pid)      //function to initialize values for a process when a process is generated
{
    int i;
    for(i=0;i<ACTIVEPROCESSLIMIT;i++)       //loop for locating an open position in the process control table
    {
        if(processTable[i] == 0)       //finds empty spot in table
        {
            processTerminationTable[i] = 0;
            processTable[i] = pid;
            processesInTable++;

            break;
        }
    }
}

void clearRequestedR(int process, int r)        //clear all requested resources for a process and resource
{
    if(arrayOfRD[r].arrayOfProcessRequests[process] != 0)
    {
        int n = arrayOfRD[r].arrayOfProcessRequests[process];
        arrayOfRD[r].arrayOfProcessRequests[process] = 0;
        sem_wait(mutex1);
        arrayOfRD[r].request -= n;
        sem_post(mutex1);
    }
}

void removeProcessFromTable(int pid)    //function to remove a process from the process table
{
    int process = findElement(pid);
    int r;
    for(r=0;r<NUMOFRD;r++)
    {
        deallocateR(process,r,0);
        clearRequestedR(process,r);
    }
    processTable[process] = 0;
    processTerminationTable[process] = 0;
    processesInTable--;
}

int checkForRequests(int process)       //checks for requests from processes
{
    int r;
    for(r=0;r<NUMOFRD;r++)
    {
        if(arrayOfRD[r].arrayOfProcessRequests[process] != 0)
        {
            currentPid = processTable[process];
            currentResource = r;
            int n = arrayOfRD[r].arrayOfProcessRequests[process];
            return n;
        }
    }
    return 0;
}

int checkForTerminations()      //checks for terminations in the termination table and removes them
{
    int process;
    for(process=0;process<ACTIVEPROCESSLIMIT;process++)
    {
        if(processTerminationTable[process] == 1)
        {
            int pid = processTable[process];
            removeProcessFromTable(pid);
//            printf("Parent: Terminating Process2: %d\n",pid);
        }
    }
}

void terminateProcess(int pid)      //will forcibly terminate a process by adding it to the termination table and sending the sigterm signal to process
{
    int process = findElement(pid);

    if(*terminationInProgress == 0)
    {
//        printf("Parent: Forcing Process %d to terminate:\n",pid);
        *terminationInProgress += 1;
        sem_post(mutex2);

        sem_wait(mutex0);
        if(*logFileLineCount < LOGFILELINELIMIT)
        {
            output = fopen(outputFileName, "a");
            fprintf(output,"Master killing deadlocked process P%d at time",process);
            printClock();
            *logFileLineCount += 1;
            fprintf(output,"Resources released are");
            int r;
            for(r=0;r<NUMOFRD;r++)
            {
                if(arrayOfRD[r].arrayOfProcessAllocation[process] > 0)
                {
                    fprintf(output," R%d:%d,",r,arrayOfRD[r].arrayOfProcessAllocation[process]);
                }
            }
            fprintf(output,"\n");
            *logFileLineCount += 1;
            fclose(output);
        }
        incrementClock(0,getRandomNumber(10000,500000));
        *changesMade = 1;
        sem_post(mutex0);

        kill(pid,SIGTERM);
//        printf("Parent: Terminating Process1: %d\n",pid);
    } else
    {
        sem_post(mutex2);
//        printf("Deadlock skipping termination of %d since a termination is in progress: %d\n",pid,*terminationInProgress);

        sem_wait(mutex0);
        if(*logFileLineCount < LOGFILELINELIMIT)
        {
            output = fopen(outputFileName, "a");
            fprintf(output,"Deadlock resolution skipping termination of P%d since a termination is currently in progress at time",process);
            printClock();
            *logFileLineCount += 1;
            fclose(output);
        }
        incrementClock(0,getRandomNumber(10000,500000));
        *changesMade = 1;
        sem_post(mutex0);
    }
}

int processLaunchCheck() //checks if program is under limit of max processes so program can generate more--returns 1 if good / returns 0 if not ok
{
    if(activeChildren < ACTIVEPROCESSLIMIT)
    {
        if(*clockSec > nextProcessLaunchTimeSec)
        {
            return 1;
        } else if(*clockSec == nextProcessLaunchTimeSec && *clockNano >= nextProcessLaunchTimeNSec)
        {
            return 1;
        } else return 0;
    } else return 0;
}

void sharedMemorySetup()        //function to setup shared memory
{
    shmid0 = shmget(SHMKEY, SIZEOFSEM, 0600 | IPC_CREAT);            //creates shared memory id for the semaphore
    if (shmid0 == -1)
    {
        perror("Shared memory0P:");                          //if shared memory does not exist print error message and exit
        exit(1);
    }
    mutex0 = (sem_t*)shmat(shmid0,NULL,0);                    //attaches semaphore to shared memory segment
    if (mutex0 == (sem_t*)-1)
    {
        perror("Mutex shmat 0P:");                           //if shared memory does not exist print error message and exit
        exit(1);
    }
    sem_init(mutex0,1,1);                                     //set semaphore initially to 1

    shmid1 = shmget(SHMKEY+1, SIZEOFSEM, 0600 | IPC_CREAT);            //creates shared memory id for the semaphore
    if (shmid1 == -1)
    {
        perror("Shared memory1P:");                          //if shared memory does not exist print error message and exit
        exit(1);
    }
    mutex1 = (sem_t*)shmat(shmid1,NULL,0);                    //attaches semaphore to shared memory segment
    if (mutex1 == (sem_t*)-1)
    {
        perror("Mutex shmat 1P:");                           //if shared memory does not exist print error message and exit
        exit(1);
    }
    sem_init(mutex1,1,1);                                     //set semaphore initially to 1

    shmid2 = shmget(SHMKEY+2, SIZEOFCLOCKSIM, 0600 | IPC_CREAT);     //creates shared memory id for the fixed size array
    if (shmid2 == -1)
    {
        perror("Shared memory2P:");                           //if shared memory does not exist print error message and exit
        exit(1);
    }
    clockSim = (int *)shmat(shmid2, 0, 0);                  //attaches to the fixed shared memory array
    clockSim[0] = (unsigned int)0;                          //initializing clock simulator
    clockSim[1] = (unsigned int)0;
    clockSim[2] = 0;
    clockSec = &clockSim[0];                                 //creates a pointer to the clock simulator to be used in the increment clock function
    clockNano = &clockSim[1];
    terminationInProgress = &clockSim[2];
    *terminationInProgress = 0;
    changesMade = &clockSim[3];
    *changesMade = 0;
    verbose = &clockSim[4];
    *verbose = v;
    logFileLineCount = &clockSim[5];
    *logFileLineCount = 0;

    shmid3 = shmget(SHMKEY+3, RDSIZE, 0600 | IPC_CREAT);     //creates shared memory id for the variable size array
    if (shmid3 == -1)
    {
        perror("Shared memory3P:");                           //if shared memory does not exist print error message and exit
        exit(1);
    }
    arrayOfRD = (rd *)shmat(shmid3, 0, 0);

    shmid4 = shmget(SHMKEY+4, RDSIZE2, 0600 | IPC_CREAT);    //creates shared memory id for the variable size array
    if (shmid4 == -1)
    {
        perror("Shared memory4P:");                                  //if shared memory does not exist print error message and exit
        exit(1);
    }
    processTable = (int*)shmat(shmid4, 0, 0);

    shmid5 = shmget(SHMKEY+5, RDSIZE3, 0600 | IPC_CREAT);    //creates shared memory id for the variable size array
    if (shmid5 == -1)
    {
        perror("Shared memory5P:");                                  //if shared memory does not exist print error message and exit
        exit(1);
    }
    processTerminationTable = (int*)shmat(shmid5, 0, 0);

    shmid6 = shmget(SHMKEY+6, SIZEOFSEM, 0600 | IPC_CREAT);       //creates shared memory id for the semaphore
    if (shmid6 == -1)
    {
        perror("Shared memory6P:");                         //if shared memory does not exist print error message and exit
        exit(1);
    }
    mutex2 = (sem_t*)shmat(shmid6,NULL,0);                   //attaches semaphore to shared memory segment
    if (mutex2 == (sem_t*)-1)
    {
        perror("Mutex shmat 6P:");                          //if shared memory does not exist print error message and exit
        exit(1);
    }
    sem_init(mutex2,1,1);
}

int deadlockCheckProcessRequestsFor0(int process)       //checks a process if it has zero requests
{
    int i;
    for(i=0;i<NUMOFRD;i++)
    {
        if(arrayOfRD[i].arrayOfProcessRequests[process] != 0)
        {
            return 0;
        }
    }
    return 1;
}

int deadlockCheckProcessAllocationForValue(int process)     //checks if process is holding allocated resources
{
    int i;
    for(i=0;i<NUMOFRD;i++)
    {
        if(arrayOfRD[i].arrayOfProcessAllocation[process] > 0)
        {
            return 1;
        }
    }
    return 0;
}

void deadlockCalcReleaseAllR(int process)       //for deadlock calculations to simulate if process releases its resources
{
    int i;
    for(i=0;i<NUMOFRD;i++)
    {
        deadlockRAvailableArray[i] += arrayOfRD[i].arrayOfProcessAllocation[process];
    }
}

int deadlockCheckFulfillment()      //check a process to see if it's requests can be fulfilled -- 0=request can be fulfilled   1=deadlocked process
{
    int deadlockFlag;
    int deadlockedProcesses = 0;
    int process;
    for(process=0;process<ACTIVEPROCESSLIMIT;process++)
    {
        deadlockFlag = 0;

        if(deadlockProcessArray[process] == 1)      //checks if process needs to be checked
        {
            int r;
            for(r=0;r<NUMOFRD;r++)
            {
                deadlockFlag = 0;
                if(arrayOfRD[r].arrayOfProcessRequests[process] > deadlockRAvailableArray[r])
                {
                    deadlockedProcesses++;
                    deadlockFlag = 1;

                    deadlockedList[deadlockedCount] = process;      //for stats
                    deadlockedCount++;
                    totalDeadlockedProcesses++;
                    break;
                }
            }
            if(deadlockFlag == 0)
            {
                deadlockCalcReleaseAllR(process);
                deadlockProcessArray[process] = 0;
                numProcLeftToCheck--;
            }
        }
    }
    if(deadlockedProcesses > 0 && (numProcLeftToCheck == deadlockedProcesses))
    {
        numProcLeftToCheck = 0;
        return 1;
    }
    return 0;
}

int deadlockDetection()     //function to check for deadlocks
{
    deadlockAbort = 0;
    numProcLeftToCheck = ACTIVEPROCESSLIMIT;

    int i;
    for(i=0;i<ACTIVEPROCESSLIMIT;i++)
    {
        deadlockedList[i] = -1;
    }
    deadlockedCount = 0;

    int r;
    for(r=0;r<ACTIVEPROCESSLIMIT;r++)
    {
        deadlockProcessArray[r] = 1;
    }
    for(r=0;r<NUMOFRD;r++)
    {
        if(arrayOfRD[r].sharable == 0 && arrayOfRD[r].available != arrayOfRD[r].total)
        {
            deadlockRAvailableArray[r] = 0;
        } else
        {
            deadlockRAvailableArray[r] = arrayOfRD[r].available;
        }
    }
    int process;
    for(process=0;process<ACTIVEPROCESSLIMIT;process++)         //check for zero requests in a process
    {
        int requestIs0 = deadlockCheckProcessRequestsFor0(process);
        if(requestIs0 == 1)
        {
            deadlockCalcReleaseAllR(process);
            deadlockProcessArray[process] = 0;
            numProcLeftToCheck--;
            int allocationHasAValue = deadlockCheckProcessAllocationForValue(process);
            if(allocationHasAValue == 1)
            {
                deadlockedList[deadlockedCount] = process;
                deadlockedCount++;
                totalDeadlockedProcesses++;
            }
        }
    }
    int deadlock = 0;
    while(numProcLeftToCheck > 0)       //check each remaining process
    {
        deadlock = deadlockCheckFulfillment();
    }
    totalTimesDeadlockDetectionHasRun++;
        if(deadlock == 1 && deadlockAbort == 0)
        {
            return 1;
        }
    return 0;
}

int calcWhichProcessHasTheMostR()   //for deadlock resolution to find the process holding the most resources
{
    int pid = processTable[0];
    int tempNumOfResources;
    int highestNumOfResources=0;
    int process;
    for(process=0;process<ACTIVEPROCESSLIMIT;process++)         //check for zero requests in a process
    {
        tempNumOfResources = 0;
        int r;
        for(r=0;r<NUMOFRD;r++)
        {
            tempNumOfResources += arrayOfRD[r].arrayOfProcessAllocation[process];
        }
        if(tempNumOfResources > highestNumOfResources)
        {
            pid = processTable[process];
            highestNumOfResources = tempNumOfResources;
        }
    }
//    printf("Process with the most R:%d  R:%d\n",pid,highestNumOfResources);
    return pid;
}

void resolveDeadlock()      //function to resolve the deadlock by terminating the process with the most resources allocated to it
{
    terminateProcess(calcWhichProcessHasTheMostR());
}

void removeZombieInfo()     //after a zombie has been handled this clears the information for the process and updates counters
{
    activeChildren--;                           //decrement number of active children
    sem_wait(mutex2);
    *terminationInProgress -= 1;
    sem_post(mutex2);
    zombiesToClean--;

    if(deadlockProgramStatus == 1)
    {
        deadlockProgramStatus = 0;
    }
    deadlockAbort = 1;
    checkForTerminations();
}

void ossAllocator()         //main function to check each process for a request,allocate the request, and check for deadlocks
{
    int process;
    for(process=0;process<ACTIVEPROCESSLIMIT;process++)
    {
        int amountToAllocate = checkForRequests(process);                   //check for requests
        if(amountToAllocate > 0 && *terminationInProgress == 0)
        {
            allocateR(currentPid, currentResource, amountToAllocate);   //allocate process if able to
        } else if(amountToAllocate < 0 && *terminationInProgress == 0)
        {
            int currentElement = findElement(currentPid);
            deallocateR(currentElement, currentResource, amountToAllocate);   //deallocate process if able to
        }
    }

    if(*terminationInProgress == 0)
    {
        if (*changesMade == 1)
        {
            sem_wait(mutex1);

            sem_wait(mutex0);
            if (*verbose == 1 && *logFileLineCount < LOGFILELINELIMIT)
            {
                output = fopen(outputFileName, "a");
                fprintf(output, "Master running deadlock detection at time");
                printClock();
                *logFileLineCount += 1;
                incrementClock(0, getRandomNumber(1000, 50000));
                fclose(output);
                sem_post(mutex0);
            }else
            {
                incrementClock(0, getRandomNumber(1000, 50000));
                sem_post(mutex0);
            }

            deadlockProgramStatus = deadlockDetection();
            sem_post(mutex1);

            if(deadlockProgramStatus == 1)
            {
                sem_wait(mutex0);
                if(*logFileLineCount < LOGFILELINELIMIT)
                {
                    output = fopen(outputFileName, "a");
//                printf("Deadlock detected!!\n");        //check for deadlock here
                    fprintf(output, "Processes");
                    int i;
                    for (i = 0; i < deadlockedCount; i++)
                    {
                        if (deadlockedList[i] >= 0 )
                        {
                            fprintf(output, " P%d,", deadlockedList[i]);
                        }
                    }
                    fprintf(output, " deadlocked at time");
                    printClock();
                    *logFileLineCount += 1;
                    fclose(output);
                }
                incrementClock(0, getRandomNumber(1000, 50000));
                sem_post(mutex0);

                sem_wait(mutex2);
                resolveDeadlock();
            } else
            {
                *changesMade = 0;
            }
        }
    }

    while(*terminationInProgress > 0 || zombiesToClean > 0)     //loop to wait for each process which has terminated to clear it's info
    {
        if(zombiesToClean > 0)
        {
            removeZombieInfo();
        }
    }
}

void handle_sigint()                    //signal handler for interrupt signal(Ctrl-C)
{
    displayAll();
    printf("\nProgram aborted by user\n");        //displays end of program message to user
    parentCleanup();
    kill(0,SIGTERM);
    while (waitpid(-1, NULL, WNOHANG) > 0);                             //wait until all children have terminated
    while (activeChildren > 0);

    exit(1);
}

void handle_sigalarm()          //signal handler for alarm signal(for time out condition)
{
    displayAll();
    printf("\nTimeout - Ending program\n");       //displays end of program message to user
    parentCleanup();
    kill(0,SIGTERM);
    while (waitpid(-1, NULL, WNOHANG) > 0);                             //wait until all children have terminated
    while (activeChildren > 0);

    exit(1);
}

void handle_sigchild()      //signal handler for child termination
{
    int status;

    while(waitpid(-1, &status, WNOHANG) > 0)          //handle each child that has terminated
    {
        if(WIFEXITED(status))
        {
            int exitStatus = WEXITSTATUS(status);
            if(exitStatus == 0)
            {
                totalSucessfulTerminations++;
            }else
            {
                totalDeadlockedProcessesTerminated++;
            }
        }

//        printf("Parent: Handling sigchild\n");       //displays end of program message to user
        zombiesToClean++;
    }
//    printf("Parent: exiting sigchild\n");
}


int main (int argc, char *argv[])
{
    output = fopen(outputFileName, "w");    //creates a new output file
    fclose(output);
    printf("Program running...\n");
    signal(SIGINT, handle_sigint);          //initialization of signals and which handler will be used for each
    signal(SIGALRM, handle_sigalarm);
    alarm(10);                               //will send alarm signal after 7 seconds
    signal(SIGCHLD, handle_sigchild);

    initializeMessages(argv[0]);            //function call to set error and usage message text
    opterr = 0;                             //disables some system error messages(using custom error messages so this is not needed)
    sharableProb = getRandomNumber(15,25);

    while ((opt = getopt(argc, argv, "hv")) != -1)		//loop for checking option selections
    {
        switch (opt)
        {
            case 'h':                                   //option h
                printHelp();                //function call to print help message
                break;
            case 'v':                                   //option h
                v = 1;
                break;
            case '?':                                   //check for arguments
                fprintf(stderr, "%s Unknown option character '-%c'\n", error, optopt);
                exit(EXIT_FAILURE);
            default:
                fprintf(stderr, "%s\n", usage);
                exit(EXIT_FAILURE);
        }
    }

    if(argc-optind > 0)             //Checks to make sure there is only 1 argument, if any, after options
    {
        printf("%s: Too many arguments\n",error);
        fprintf(stderr, "%s\n", usage);
        exit(EXIT_FAILURE);
    }

    sharedMemorySetup();


    //initialize the resource descriptors
    int i;
    for(i=0;i<NUMOFRD;i++)
    {
        arrayOfRD[i].total = arrayOfRD[i].available = getRandomNumber(1,10);
        arrayOfRD[i].sharable = getSharableClass();

        int j;
        for(j=0;j<arrayOfRD[i].total;j++)
        {
            arrayOfRD[i].arrayOfProcessAllocation[j] = 0;
        }

        for(j=0;j<ACTIVEPROCESSLIMIT;j++)
        {
            arrayOfRD[i].arrayOfProcessRequests[j] = 0;
        }
    }

    int pid;
    int launchprocess=0;

    getNextProcessTime();

    while(totalProcessesLaunched < TOTALPROCESSESLIMIT)         //loop that repeats until all processes have been generated
    {
        launchprocess = processLaunchCheck();

        if(launchprocess == 1)
        {
            activeChildren++;
            pid = fork();                                   //fork call
            if(pid < 0)
            {
                fprintf(stderr, "Fork failed:%s\n",strerror(errno));
                parentCleanup();
                kill(0,SIGTERM);
                while (waitpid(-1, NULL, WNOHANG) > 0);                             //wait until all children have terminated
                while (activeChildren > 0);

                exit(-1);
            }
            if (pid == 0)
            {
                execl("process", NULL);                 //exec call
            }
            loadProcessIntoTable(pid);
//            printf("Launching pid: %d\n",pid);

            sem_wait(mutex0);
            incrementClock(0,getRandomNumber(1000,2000));
            sem_post(mutex0);

            if(totalProcessesLaunched == 25)    //print a progress percent to terminal
            {
                printf("20%%\n");
            }else if(totalProcessesLaunched == 50)
            {
                printf("40%%\n");
            }else if(totalProcessesLaunched == 75)
            {
                printf("60%%\n");
            }else if(totalProcessesLaunched == 100)
            {
                printf("80%%\n");
            }

            getNextProcessTime();
            totalProcessesLaunched++;
        }
        ossAllocator();
        sem_wait(mutex0);
        incrementClock(1,getRandomNumber(1000,500000));
        sem_post(mutex0);
    }

while(activeChildren > 0 || processesInTable > 0)          //program will wait here to make sure all active child processes have been terminated
{
    ossAllocator();
    sem_wait(mutex0);
    incrementClock(1,getRandomNumber(1000,500000));
    sem_post(mutex0);
}

    displayAll();
    parentCleanup();                                                        //function call to clean up parent shared memory

    output = fopen(outputFileName, "a");
    fprintf(output,"\nProgram completed successfully\n");
    fclose(output);
    printf("Program completed successfully --> check \"%s\"\n",outputFileName);                         //completed message for user

    return 0;
}
