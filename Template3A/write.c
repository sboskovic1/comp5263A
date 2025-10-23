#include "global.h"

// NEW
extern int pendingWrite[];
extern int NUM_WAITBUFS, NUM_FU, NUM_COPIES, NUM_ITERATIONS;
extern int TRACE;

extern char *map(int);

extern struct WaitBufEntry WaitBuffer[];
extern unsigned REG_FILE[];


// Communicate with  DISPATCH stage 
extern int isFree[ ];

// Communicate with EXECUTE stage
extern int resultReady[ ];
extern struct resultEntry  resultData[ ];

// Statistics
extern int numInstrComplete;

void do_write();

void writestage()
{
  int job_num;
  job_num = ActivityArgSize(ME) - 1;
  while(1){	
    if (TRACE)
      printf("\nIn WRITE Stage at time %2.0f\n", GetSimTime());
	do_write();
	ProcessDelay(1.000);
  }
}


void do_write() {
  int fu;
  int destReg, result;
  int index;

  for (fu=1; fu < NUM_FU * NUM_COPIES; fu++) {
    if (resultReady[fu] == TRUE)
      break;
  }

  if (fu == NUM_FU * NUM_COPIES) 
    return;

    isFree[fu] = TRUE; 
    resultReady[fu] = FALSE;

    if (fu != STOREFP) {
    destReg = resultData[fu].destReg;
    result = resultData[fu].result;

    REG_FILE[destReg] = result;   
    pendingWrite[destReg] = FALSE;  // Write conflict with Issue stage. Issue sage will  get priority.
    }
    numInstrComplete++;


    if (TRACE)
      printf("\tCompleted Instruction: %s. Result: %d Dest Reg: %d Num Instructions Completed: %d\n", map(fu), result, destReg, numInstrComplete);  

}

