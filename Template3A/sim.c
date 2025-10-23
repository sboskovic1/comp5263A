#include "global.h"
#include <string.h>
#include <stdlib.h>

// Default Parameter Values
int POLICY= RANDOM;  // Eviction policy
int NUM_WAY_BITS = 0;
int  CACHE_SIZE_BITS = 8;
int NUM_THREADS = 1;

int NUM_WAYS;  // Cache Associativity
int CACHE_SIZE_BYTES;   // Size of Cache in Bytes
int CACHE_SIZE_SETS; // NumSets in Cache
int NUM_THREADS;

int usage [MAX_CACHE_SIZE_SETS][MAX_NUM_WAYS]; // Used to implement pseuso-LRU eviction policy

// Cache Statistics
int cache_hits[MAX_NUM_THREADS], cache_misses[MAX_NUM_THREADS];
int cache_reads[MAX_NUM_THREADS], cache_writes[MAX_NUM_THREADS];
int cache_writebacks[MAX_NUM_THREADS];

unsigned PHYMEM[MEM_SIZE_INTS];
unsigned * PhysicalBaseAddress; // Array A will begin at this address

  int usage [MAX_CACHE_SIZE_SETS][MAX_NUM_WAYS]; // Used to implement pseuso-LRU eviction policy

// IF Stage
unsigned insMEM[INSTR_MEM_SIZE];
unsigned PC, PC4;
unsigned INSTRUCTION;
unsigned nextPC;
int stallIF;
unsigned branchFlag;
int isHALT;

// Issue Stage
int scoreBoard[MAX_NUM_WAITBUFS][NUM_REGISTERS];
int pendingWrite[NUM_REGISTERS];
int pendingRead[NUM_REGISTERS];
struct WaitBufEntry  insWaitBuffer[MAX_NUM_WAITBUFS];
unsigned REG_FILE[NUM_REGISTERS];

struct LSQEntry  LSQ[MAX_NUM_LSQ_SLOTS];
int LSQhead, LSQtail, LSQcount;
int numLSQfullStallCycles;


// Dispatch Stage
int isFree[MAX_NUM_FU * MAX_NUM_COPIES];
unsigned  MEM[MEM_SIZE];
struct workEntry myWork[MAX_NUM_FU * MAX_NUM_COPIES];
int workAvail[MAX_NUM_FU * MAX_NUM_COPIES];

// Exec and Write Stages
SEMAPHORE * sem_myRS[MAX_NUM_FU * MAX_NUM_COPIES];
int resultReady[MAX_NUM_FU * MAX_NUM_COPIES];
struct resultEntry  resultData[MAX_NUM_FU * MAX_NUM_COPIES];


// Statistics
int numWAWStallCycles,numWARStallCycles;
int numInstrComplete;
int numStallCycles, numBranchStallCycles, numHaltStallCycles, numBranchDataStallCycles, numWaitBufferFullStallCycles;
int   numStallFUAvail[MAX_NUM_FU * MAX_NUM_COPIES];
double timeInsCompleted, timeInsRetired;
int numLSQfullStallCycles;

// Pipeline stage Processes
PROCESS *fetch, *issue, *dispatch, *execute, *write;
PROCESS  *display;
PROCESS *FU;

extern void flushCache();
extern void showRegFile();
extern void reset();
extern   void fetchstage(), issuestage(), dispatchstage(), executestage(), writestage(), displaystage();
extern	void FUs();
extern void showRegFile();
extern void showWaitBuffer();
extern char *map(int);

int NUM_WAITBUFS, NUM_FU, NUM_COPIES, NUM_ITERATIONS, TRACE;


void getparams(int argc, char *argv[]) {
  int i;

    for (i=1; i < argc ; i = i+2) {
      if (strcmp(argv[i], "--numIterations") == 0) {
	NUM_ITERATIONS = atoi(argv[i+1]);
      }
      else  if (strcmp(argv[i], "--numWaitBuffers") == 0){
	NUM_WAITBUFS = atoi(argv[i+1]);
      }
      else  if (strcmp(argv[i], "--numFUs") == 0) {
	NUM_FU = atoi(argv[i+1]);
      }
      else  if (strcmp(argv[i], "--numCopies") == 0){
	NUM_COPIES = atoi(argv[i+1]);
      }
      else  if (strcmp(argv[i], "--trace") == 0){
	TRACE  = atoi(argv[i+1]);
      }
      
      else     if (strcmp(argv[i], "--evictPolicy") == 0) {
	if (strcmp(argv[i+1], "PLRU") == 0)
	  POLICY = PLRU;
	else 	if (strcmp(argv[i+1], "RANDOM") == 0)
	  POLICY = RANDOM;
	else {
	  printf("Unsupported Eviction Policy\n");
	  exit(1);
	}
      }

    else  if (strcmp(argv[i], "--waysBits") == 0){
     NUM_WAY_BITS  = atoi(argv[i+1]);
    }
    else  if (strcmp(argv[i], "--cachesizeBits") == 0) {
      CACHE_SIZE_BITS = atoi(argv[i+1]);
    }
    else  if (strcmp(argv[i], "--numThreads") == 0){
      NUM_THREADS  = atoi(argv[i+1]);
    }
    else {
	  printf("Unmatched Argument. BYE!!!\n");
	  exit(1);
      }
    }
}


void UserMain(int argc, char *argv[]){
  int i, j;

  getparams(argc, argv);

  NUM_COPIES = 1;
  NUM_WAITBUFS = 4;
  NUM_FU = 8;

  NUM_WAYS = (0x1 << NUM_WAY_BITS);  // Cache Associativity
  CACHE_SIZE_BYTES = (0x1 << CACHE_SIZE_BITS);  // Size in Bytes
  CACHE_SIZE_SETS =  (CACHE_SIZE_BYTES/(NUM_WAYS * BLK_SIZE_BYTES));

  printf("\n******************************************************************************\n");
  printf("Num_ITERATIONS: %d\tNUM_FUs: %d\tNUM_COPIES: %d\tNUM_WAITBUFS: %d\tNUM_LSQ_SLOTS: %d\n", NUM_ITERATIONS, NUM_FU, NUM_COPIES, NUM_WAITBUFS, MAX_NUM_LSQ_SLOTS);
    printf("******************************************************************************\n");
  printf("Cache Size (Bytes): %d\t Number Ways: %d\tBLK_SIZE (bytes): %d\n",  (0x1 << CACHE_SIZE_BITS), (0x1 <<NUM_WAY_BITS), BLK_SIZE_BYTES);
    printf("******************************************************************************\n");


    reset();

    for (i=0; i < MAX_NUM_FU * MAX_NUM_COPIES; i++)
      sem_myRS[i] = NewSemaphore("RSindex",0);

    numInstrComplete = 0;
    numStallCycles = 0;
    numWAWStallCycles = 0;
    numWARStallCycles = 0;     
    for (i=0; i < MAX_NUM_FU * MAX_NUM_COPIES; i++) 
      numStallFUAvail[i] = 0;

    // create a process for each pipeline stage

    write = NewProcess("write",writestage,0);
    ActivitySetArg(write,NULL,1);
    ActivitySchedTime(write,0.000000,INDEPENDENT);

    execute = NewProcess("execute",executestage,0);
    ActivitySetArg(execute,NULL,1);
    ActivitySchedTime(execute,0.000001,INDEPENDENT);


    dispatch = NewProcess("dispatch",dispatchstage,0);
    ActivitySetArg(dispatch,NULL,1);
    ActivitySchedTime(dispatch,0.00002,INDEPENDENT);


    issue = NewProcess("issue",issuestage,0);
    ActivitySetArg(issue,NULL,1);
    ActivitySchedTime(issue,0.00003,INDEPENDENT);


    fetch = NewProcess("fetch",fetchstage,0);
    ActivitySetArg(fetch,NULL,1);
    ActivitySchedTime(fetch,0.00004,INDEPENDENT);


    for (j=0; j < NUM_COPIES; j++) {
      for (i=1; i < NUM_FU; i++) {
	FU = NewProcess("fui", FUs,0);
	ActivitySetArg(FU,NULL,j*NUM_FU + i);
	ActivitySchedTime(FU,0.00003,INDEPENDENT);
      }
    }

    // create a display process to print out pipeline state

    display = NewProcess("display",displaystage,0);
    ActivitySetArg(display,NULL,1);
    ActivitySchedTime(display,0.00004,INDEPENDENT);


    // Initialization is done, now start the simulation

	DriverRun(MAX_SIMULATION_TIME);

	printf("\n****************************************************************************************\n");
	printf("Simulation ended  at %3.0f\n",GetSimTime());
	printf("Execution completed at time  %3.0f\n",timeInsCompleted);
	printf("Retirement completed  at time  %3.0f\n",timeInsRetired);

	printf("\nNumber Instructions Retired: %d\n", numInstrComplete);
	printf("Number Stall Cycles: %d\n", numStallCycles);
	printf("Number Stall Cycles: Halt: %d\tWaitBufferFull: %d\tBranchControl: %d\tBranch Data: %d\n", numHaltStallCycles, numWaitBufferFullStallCycles, numBranchStallCycles, numBranchDataStallCycles);

	for (i=0; i < NUM_FU * NUM_COPIES; i++) {
	  if (numStallFUAvail[i] > 0)
	    printf("Number Stall Cycles %s: %d\n", map(i), numStallFUAvail[i]);
	}
	printf("Number WAW Stall Cycles: %d\n", numWAWStallCycles);
	printf("Number WAR Stall Cycles: %d\n", numWARStallCycles);

	printf("\nNumber Cache Hits: %d\n",  cache_hits[0]);
	printf("NUMBER Cache Misses: %d\n", cache_misses[0]);
	printf("NUMBER Cache Write Backs: %d\n", cache_writebacks[0]);

	showRegFile();

	  printf("\nFinal Destination Memory Array\n");
	  	  	  for (i=0; i < NUM_ITERATIONS ; i = i + 1)
	  	  printf("MEM[%x]: %d\n", PhysicalBaseAddress+0x100+i,*(PhysicalBaseAddress+0x100+i) );
}

