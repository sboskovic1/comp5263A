#include "global.h"

extern unsigned INSTRUCTION, PC4;  // Input from FETCH stage
extern unsigned stallIF, branchFlag, nextPC;  // Output back to FETCH stage

extern struct WaitBufEntry  insWaitBuffer[ ];
extern unsigned REG_FILE[ ];
extern int scoreBoard[][NUM_REGISTERS];
extern int pendingWrite[];
extern int  pendingRead[];

// Statistics 
extern int numWAWStallCycles, numWARStallCycles;  // Statistics
extern int numInstrComplete;
extern int  numBranchStallCycles, numHaltStallCycles;
extern int  numBranchDataStallCycles, numWaitBufferFullStallCycles;
// NEW Project 3
extern struct LSQEntry  LSQ[];
extern  int LSQhead;
extern  int LSQtail;
extern int  LSQcount;
extern int numLSQfullStallCycles;

extern void updatePendingReads(int);
extern int showWaitBuffer(int);
extern char * bool(int);
extern char * map(int);

extern int NUM_WAITBUFS, NUM_FU, NUM_COPIES, NUM_ITERATIONS, TRACE;
extern unsigned isHALT; // Used to control the output display only 

void do_issue();
void setSource1(int, int, int);
void setSource2(int, int, int);
void setDestination(int, int);
void setSource1LSQ(int, int, int);
void setSource2LSQ(int, int, int);
void setDestinationLSQ(int, int);
/****************************************************************************** */

int getFU(int op) {  // Return index of next FU for that op without checking its  free/busy status
  int i, fu;
  static int nextFU[MAX_NUM_FU] = {0};

  nextFU[op] = (nextFU[op]+1) % NUM_COPIES;
  fu =  nextFU[op] * NUM_FU + op;
  if (DEBUG2)
    printf("Selected FU: Copy %d  opcode: %d  FU Id: %d\n", nextFU[op], op, fu); 
  return(fu);
}

int getFreeWaitBufferEntry() {
  int i;
    for (i=0; i < NUM_WAITBUFS; i++) 
    if (insWaitBuffer[i].free == TRUE)
      return(i);

  return(-1);
}

// NEW
int getFreeLSQEntry() {
  int temp;
 
  if (LSQcount == MAX_NUM_LSQ_SLOTS)
    return (-1);
  else {
    temp = LSQtail;
    LSQtail = (LSQtail+1)%MAX_NUM_LSQ_SLOTS;
    LSQcount++;
    return temp;
  }
} 

void decode(int *opCode, int *srcReg1, int *srcReg2, int *destReg, int *offset) {
  unsigned instr, op;
  
  instr = INSTRUCTION;
  *opCode = (instr >> 26) & 0x3F; // INSTRUCTION[31:26]
  *srcReg1 = (instr >> 21) & 0x1F; // INSTRUCTION[25:21];
  *srcReg2 = (instr >> 16) & 0x1F; //INSTRUCTION[20:16];
  *destReg = (instr >> 11) & 0x1F; //INSTRUCTION[15:11];
  *offset =  instr & 0xFFFF; // //INSTRUCTION[15:0];
  if ( (instr >> 15) & 0x1)  //  sign extend offset
    *offset =0xFFFF0000 | *offset;   
  nextPC = PC4 + *offset;  
}

void issuestage() {
  int job_num;
  job_num = ActivityArgSize(ME) - 1;
    
  while(1) {
    if (TRACE)
      printf("In ISSUE  Stage at time %2.0f\n", GetSimTime());
    do_issue();
    ProcessDelay(1.0);	
  }
}

 
void do_issue() {
  int opCode, srcReg1, srcReg2,destReg,offset;
  int waitBufIndex;
  int fu;
  int LSQIndex;   // NEW

  stallIF = FALSE;  
  branchFlag = FALSE;

  decode(&opCode, &srcReg1, &srcReg2, &destReg, &offset);

  if (opCode == NOP) {
    if (DEBUG) 
      printf("\tInstruction is NOP. Will not issue into WaitBuffer. Time: %5.2f\n", GetSimTime());
    return;
  }

  if (opCode == HALT) {
        if (DEBUG) 
	  printf("\tInstruction is HALT. Will not issue into WaitBuffer. Will assert stallIF. Time: %5.2f\n", GetSimTime());
	stallIF = TRUE;
	isHALT = TRUE;
	numHaltStallCycles++;
    return;
  }


  if (opCode == BRANCH) {
    branchFlag = TRUE;
    numInstrComplete++;
    if (DEBUG)
      printf("\tCompleted Instruction: %s. Number Instructions Completed: %d Time: %5.2f\n", "BRANCH", numInstrComplete, GetSimTime());
    return;
  }

  
  if (opCode == BNEZ) {
    if  (pendingWrite[srcReg1] == 0) {  // No writer 
	    if (REG_FILE[srcReg1] != 0)  // Taken Branch
	      branchFlag = TRUE;
	    numInstrComplete++;
	    if (DEBUG)
	      printf("\tCompleted Instruction: %s.  Time: %5.2f Number Instructions Completed: %d\n", "BNEZ", GetSimTime(), numInstrComplete);
	    return;
	}
	else {
	  stallIF = TRUE;
	  numBranchDataStallCycles++;
	  if (DEBUG)
	    printf("\tBNEZ source register %d not READY ! Setting IFstall. Time: %5.2f\n", srcReg1,  bool(stallIF), GetSimTime());
	  return;
	}
  }

  /* Handle WAW and WAR Hazards*/

  if (opCode != STOREFP) {
    if (pendingWrite[destReg] != 0) {  
      stallIF = TRUE;
      numWAWStallCycles++;
      if (TRACE)  
	printf("\tWAW Stall. Time: %5.2f  OP: %s Register: %d Num WAW Stalls: %d\n", GetSimTime(), map(opCode),  destReg,  numWAWStallCycles);
      return;
    }
  
    updatePendingReads(destReg);
    if (pendingRead[destReg]) {
      stallIF = TRUE;
      numWARStallCycles++;
      if (TRACE)  
	printf("\tWAR Stall.Time: %5.2f  OP: %s Register: %d Num WAR Stalls: %d\n", GetSimTime(), map(opCode),   destReg, numWARStallCycles);
      return;
    }
  }


/* Instruction does not have WAW or WAR dependency  */

  fu = getFU(opCode);  
  if ( (opCode != LOADFP) &&  (opCode != STOREFP) ) {
    if ( ( waitBufIndex = getFreeWaitBufferEntry()) == -1) {
      stallIF = TRUE;
      numWaitBufferFullStallCycles++;
      if (DEBUG)
	printf("\tNo free insWaitBuffer entries! Setting stallIF. Time: %5.2f\n", GetSimTime());
      return;
    }
  insWaitBuffer[waitBufIndex].free = FALSE; 
  insWaitBuffer[waitBufIndex].fu = fu;
  }
  else {
    if ( ( LSQIndex = getFreeLSQEntry()) == -1) {
      stallIF = TRUE;
      numLSQfullStallCycles++;
      if (DEBUG)
	printf("\tNo free LSQBuffer entries! Setting stallIF. Time: %5.2f\n", GetSimTime());
      return;
    }
  LSQ[LSQIndex].free = FALSE; 
  LSQ[LSQIndex].fu = fu;
  }

// Set the source1, source2, destReg, op1Ready and op2Ready fields of the insWaitBuffer entry in whch this instruction is being issued.
// Note that some instructions do not  have a source2 and some do not have a destReg.
// op1Ready and op2Ready re special flags that override the default settings for a RAW dependency.

  switch(opCode) {
  case ADDFP:
  case SUBFP:
  case MULFP: 
  case INTADD:
    setSource1(waitBufIndex, srcReg1, destReg);
    setSource2(waitBufIndex, srcReg2, destReg);
    setDestination(waitBufIndex, destReg);
    break;
   
  case LOADFP:
    setSource1LSQ(LSQIndex, srcReg1, destReg);
    LSQ[LSQIndex].srcReg2 = -1;   // No source 2 register 
    setDestinationLSQ(LSQIndex, destReg);
    break;

  case STOREFP:
    setSource1LSQ(LSQIndex, srcReg1, -1);
    setSource2LSQ(LSQIndex, srcReg2, -1);
    LSQ[LSQIndex].destReg = -1;  // No destination register 
  break;
  }

  if (TRACE) {
    if ((fu != LOADFP) && (fu !=STOREFP)) 
      printf("\tOPCODE: %s  Adding to insWaitBuffer[%d]: src1Reg1: %d srcReg2: %d destReg: %d\n", map(fu),waitBufIndex, insWaitBuffer[waitBufIndex].srcReg1, insWaitBuffer[waitBufIndex].srcReg2, insWaitBuffer[waitBufIndex].destReg); 
    else
      printf("\tOPCODE: %s  Adding to LSQ[%d]: src1Reg1: %d srcReg2: %d destReg: %d\n", map(fu),LSQIndex, LSQ[LSQIndex].srcReg1, LSQ[LSQIndex].srcReg2, LSQ[LSQIndex].destReg); 
  }
  if (DEBUG2) {
    printf("\t\tISSUE stage: pendingWrite[%d]: %d\n", destReg, pendingWrite[destReg]); 
    showWaitBuffer(waitBufIndex);
  } 
}
  
void setSource1(int waitBufIndex, int  srcReg1, int destReg) {
  scoreBoard[waitBufIndex][srcReg1] = TRUE;
  insWaitBuffer[waitBufIndex].srcReg1 = srcReg1; 
  if (srcReg1 == destReg)
    insWaitBuffer[waitBufIndex].op1Ready = TRUE;  // Override false RAW dependency
  else
    insWaitBuffer[waitBufIndex].op1Ready = FALSE;
}

void setSource2(int waitBufIndex, int srcReg2, int destReg) {
  scoreBoard[waitBufIndex][srcReg2] = TRUE;
  insWaitBuffer[waitBufIndex].srcReg2 = srcReg2; 
  if (srcReg2 == destReg)
    insWaitBuffer[waitBufIndex].op2Ready = TRUE;
  else
    insWaitBuffer[waitBufIndex].op2Ready = FALSE;
}


void setDestination(int waitBufIndex, int destReg) {
  pendingWrite[destReg]= TRUE; 
  insWaitBuffer[waitBufIndex].destReg = destReg;  
}  


void setSource1LSQ(int LSQIndex, int  srcReg1, int destReg) {
  scoreBoard[LSQIndex + NUM_WAITBUFS][srcReg1] = TRUE;
  LSQ[LSQIndex].srcReg1 = srcReg1; 
  if (srcReg1 == destReg)
    LSQ[LSQIndex].op1Ready = TRUE;  // Override false RAW dependency
  else
    LSQ[LSQIndex].op1Ready = FALSE;
}

void setSource2LSQ(int LSQIndex, int srcReg2, int destReg) {
  scoreBoard[LSQIndex + NUM_WAITBUFS][srcReg2] = TRUE;
  LSQ[LSQIndex].srcReg2 = srcReg2; 
  if (srcReg2 == destReg)
    LSQ[LSQIndex].op2Ready = TRUE;
  else
    LSQ[LSQIndex].op2Ready = FALSE;
}


void setDestinationLSQ(int LSQIndex, int destReg) {
  pendingWrite[destReg]= TRUE; 
  LSQ[LSQIndex].destReg = destReg;  
}  


