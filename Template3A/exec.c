#include "global.h"

extern int isFree[ ], workAvail[];   // Communicate with  DISPATCH stage
extern struct workEntry  myWork[ ];

extern int resultReady[ ];  // Communicate with  WRITE  stage
extern struct resultEntry  resultData[ ];

extern unsigned MEM[];  // Data Memory

extern int NUM_WAITBUFS, NUM_FU, NUM_COPIES, NUM_ITERATIONS, TRACE;
extern SEMAPHORE * sem_myRS[ ];

extern char *map(int);
extern 	int cacheRead(int, int, int *, int *);
extern 	int cacheWrite(int, int, int, int *, int *);

void do_execute();

void executestage(){
  int fu_id;
  fu_id = ActivityArgSize(ME) - 1;
  while(1){	
    if (TRACE)
      printf("In EXECUTE Stage at time %2.0f\n", GetSimTime());
    do_execute();
    ProcessDelay(1.000);
  }
}

void do_execute() {
  int i;

  for (i=0; i < NUM_FU * NUM_COPIES; i++) {
    if (workAvail[i]) {
      SemaphoreSignal(sem_myRS[i]);
      workAvail[i] = FALSE; 
      //      break;
    }
  }
}

void FUs(){
  int  fu, operand1, operand2, operation, result, addr;
  int i;

  unsigned  cacheIndex, cacheWay; // Used for printing only


  fu = ActivityArgSize(ME); // Yacsim argument 
  while(1){
    SemaphoreWait(sem_myRS[fu]); // Wait for work
    isFree[fu] = FALSE;

    if (TRACE)
      printf("\tFU  %s (id: %d) woken up at time %2.0f\n", map(fu), fu, GetSimTime());	 
    
    operand1 = myWork[fu].operand1;
    operand2 = myWork[fu].operand2;
    operation = fu % NUM_FU;


    if (TRACE)
      printf("FU %s  Starting: operand1: %d  operand2: %d\n", map(fu), operand1, operand2);
    
    switch (operation) {
    case ADDFP: 
      ProcessDelay(( (double) ADDFP_CYCLES) - epsilon);
      result = operand1 + operand2;   
      break;
		  
    case SUBFP: 
      ProcessDelay( (double) SUBFP_CYCLES - epsilon);
      result = operand1 - operand2;   
      break;
		  
    case MULFP: 
      ProcessDelay((double) MULFP_CYCLES - epsilon);
      result = operand1 * operand2;   
      break;
		 		   
    case LOADFP:    
           	addr = (unsigned) operand1;
		if (TRACE)
		  printf("Doing LOAD addr: %x at %5.2f\n", addr, GetSimTime());
		result = cacheRead(0, addr, &cacheIndex, &cacheWay);
		if (TRACE)
		  printf("Completed LOAD addr: %x at %5.2f\n", addr, GetSimTime());
	break;
     
	/*

    ProcessDelay((double) LOADFP_CYCLES - epsilon);   
      addr = (unsigned) operand1;
      result = MEM[addr];
      break;
	*/
  
    case INTADD: 
      result = operand1 + operand2;   
      ProcessDelay(( (double) INTADD_CYCLES) - epsilon);
      break;	  
		  
    case STOREFP: 
   	// Handle Store
	result = operand2;// Store value 
	addr = (unsigned) operand1;
	if (TRACE)
	  printf("Doing STORE addr: %x at %5.2f\n", addr, GetSimTime());
	cacheWrite(0, result, addr, &cacheIndex, &cacheWay); 
	if (TRACE)
	  printf("Completed STORE addr: %x at %5.2f\n", addr, GetSimTime());
	break; 
   
      /*      printf("Doing a STORE at %5.2f\n", GetSimTime());
      ProcessDelay((double) STOREFP_CYCLES - epsilon);
      result = operand2;// Store value -- No real result
      addr = (unsigned) operand1;
      MEM[addr] = operand2;
      break; 
      }
      */
    }
	
    if (TRACE)
      printf("Time: %5.2f FU completed %s: operand1: %d  operand2: %d  result: %d\n", GetSimTime(), map(fu), operand1, operand2, result);				  

    while (resultReady[fu] == TRUE) { // Busy wait for write stage to process earlier fu result
      ProcessDelay(1.0);
    }
    resultData[fu].result = result;  // Move fu results to write stage
    resultData[fu].destReg = myWork[fu].destReg;  
    resultReady[fu] = TRUE;
    if (TRACE) 
      printf("\t\tFU Unit %s delivered results  at time %5.2f-\n", map(fu), GetSimTime());
  }
}


