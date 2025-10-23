#include "global.h"

extern int NUM_WAITBUFS, NUM_FU, NUM_COPIES, NUM_ITERATIONS;
extern int TRACE;

extern char * bool(int);
extern unsigned insMEM[];  // Instruction Memory
extern int numStallCycles;  // Statistics Counter


// Input Signals set by  ISSUE stage
extern int stallIF, branchFlag;
extern unsigned nextPC;

// Output Signals for ISSUE stage
extern unsigned  INSTRUCTION, PC4;
extern unsigned  PC;  

void do_fetch();

void fetchstage() {
        int job_num;
	job_num = ActivityArgSize(ME) - 1;
	
	while(1){
	  if (TRACE)
	    printf("In FETCH  Stage at time %2.0f\n", GetSimTime());
	do_fetch();
	ProcessDelay(1.0);
	}
}


void do_fetch() {
  if (!stallIF) {
    if (branchFlag) {
      INSTRUCTION = NOP;
      PC = nextPC;    
    }
    else {
      INSTRUCTION  = insMEM[PC/4];
      PC = PC + 4;
    }
    PC4 = PC;
  }
  else {
    numStallCycles++;
    if (DEBUG)
      printf("\tstallIF: %s  INSTRUCTION: %x\n", bool(stallIF), INSTRUCTION);
  }

 if (TRACE) 
    printf("\tPC: %d INSTRUCTION: %x\n", GetSimTime(), PC, INSTRUCTION);
}


