#include "global.h"

extern struct WaitBufEntry  insWaitBuffer[]; // From ISSUE stage
extern struct LSQEntry  LSQ[]; // From ISSUE stage
extern unsigned REG_FILE[ ];
extern int scoreBoard[][NUM_REGISTERS]; 
extern int pendingWrite[], pendingRead[];
extern int isFree[];  // Set in   EXECUTE and WRITE stages
extern int workAvail[ ];   // To EXEC stge 
extern struct workEntry  myWork[ ]; // To EXEC Stage

extern int   numStallFUAvail[]; // Statistics

extern int NUM_WAITBUFS, NUM_FU, NUM_COPIES, NUM_ITERATIONS, TRACE;
extern char * map(int);


// NEW
extern struct LSQEntry  LSQ[];
extern  int LSQhead;
extern  int LSQtail;
extern int  LSQcount;
extern int numLSQfullStallCycles;

void do_dispatch();
void do_LSQ_dispatch();
/****************************************************************************** */

void updatePendingReads(int srcReg) {
  int i;

  pendingRead[srcReg] = FALSE;
  for (i=0; i < NUM_WAITBUFS + MAX_NUM_LSQ_SLOTS; i++) {
    if (scoreBoard[i][srcReg] == TRUE) {
      pendingRead[srcReg] = TRUE;
      return;
    }
  }
}

void dispatchstage() {  
  int job_num;
  job_num = ActivityArgSize(ME) - 1;
  while(TRUE){	  
    if (TRACE)
      printf("In DISPATCH Stage at time %2.0f\n", GetSimTime());
    do_dispatch();
    do_LSQ_dispatch();
    ProcessDelay(1.000);
  }
}

void do_dispatch() {
  int i, j, index;
  int fu;
 int srcReg1, srcReg2, destReg;
 static int next = 0;

 for (i= next; i < next + NUM_WAITBUFS; i=i+1) {
   if (insWaitBuffer[i % NUM_WAITBUFS].free == TRUE)
     continue;  // No instruction in this slot

  // Found an WaitBuffer entry with a waiting  instruction  
   index = i % NUM_WAITBUFS;
   srcReg1 = insWaitBuffer[index].srcReg1;
   srcReg2 =  insWaitBuffer[index].srcReg2;
   destReg = insWaitBuffer[index].destReg;

   if  ( pendingWrite[srcReg1] == TRUE  &&  insWaitBuffer[index].op1Ready == FALSE)  
       continue;  // RAW dependent on an in-flight instruction
   if (srcReg2 != -1)
     if ( pendingWrite[srcReg2] == 1 &&  insWaitBuffer[index].op2Ready == FALSE )
   continue; // RAW dependent on an in-flight instruction

   // Current instruction has its RAW dependencies satisfed. 

   fu = insWaitBuffer[index].fu;
 
   if  (!isFree[fu]) {
     numStallFUAvail[fu]++;  // Statistics counter
     if (TRACE)
       printf("No FU (%s)  unit available  numStall: %d\n", map(fu), numStallFUAvail[fu]); 
     continue;  // FU not free 
   }
 
 // Operands and FU available
   myWork[fu].operand1 = REG_FILE[srcReg1];
   if (srcReg2 !=  -1) 
     myWork[fu].operand2 = REG_FILE[srcReg2];
   myWork[fu].destReg = destReg;
   workAvail[fu] = TRUE; // Signal FU 
   
	
    if (DEBUG2) {
      printf("\n**********************************\n");
      printf("operand1: %d  operand2: %d    destReg: %d\n", myWork[fu].operand1, myWork[fu].operand2, myWork[fu].destReg);
      printf("\n**********************************\n");
    }

    scoreBoard[index][srcReg1] = FALSE;  
    updatePendingReads(srcReg1);
    if (srcReg2 != -1) {
      scoreBoard[index][srcReg2] = FALSE;
      updatePendingReads(srcReg2);
    }
   
   
    if (TRACE) 
      printf("\tActivating FU %s from insWaitBuffer[%d] at time %5.2f\n", map(insWaitBuffer[index].fu), index, GetSimTime());
      
    insWaitBuffer[index].free = TRUE;
    next = (index+1) % NUM_WAITBUFS;

    if (DEBUG2)
      printf("Freeing up WaitBuffer entry %d\n", index);
    
    return;  // At most 1 instruction dispatched in any cycle
 }

 if (TRACE)
   printf("\tCould not find any instruction to dispatch. Time: %5.2f\n", GetSimTime());
}


void do_LSQ_dispatch() {
  int i, j, LSQindex;
  int fu;
 int srcReg1, srcReg2, destReg;
 static int next = 0;


    if (LSQcount == 0) {
     if (TRACE)
       printf("\tCould not find any MEM instruction to dispatch. Time: %5.2f\n", GetSimTime());
     return;
 }

   LSQindex = LSQhead;

// Entry at head of LSQ available
   srcReg1 = LSQ[LSQindex].srcReg1;
   srcReg2 =  LSQ[LSQindex].srcReg2;
   destReg = LSQ[LSQindex].destReg;

   //   printf("XXX LSQindex %d  srcReg1: %d pendingWrite[%d]: %d \n", LSQindex, srcReg1, srcReg1, pendingWrite[srcReg1]);
   //  printf("XXX LSQindex %d  srcReg2: %d pendingWrite[%d]: %d \n", LSQindex, srcReg2, srcReg2, pendingWrite[srcReg2]);
   if  (pendingWrite[srcReg1] == TRUE  &&  LSQ[LSQindex].op1Ready == FALSE)  
       return;  // RAW dependent on an in-flight instruction
   if (srcReg2 != -1) 
     if (pendingWrite[srcReg2] == 1 &&  LSQ[LSQindex].op2Ready == FALSE )
       return; // RAW dependent on an in-flight instruction

   // Current instruction has its RAW dependencies satisfed. 
   fu = LSQ[LSQindex].fu;
   if  ( !isFree[LOADFP] || !isFree[STOREFP] ) {
     numStallFUAvail[fu]++;  // Statistics counter
     if (TRACE)
       printf("MEM found no FU (%s)  unit available  numStall: %d\n", map(fu), numStallFUAvail[fu]); 
     return;  // FU not free 
   }

 
 // Operands and FU available
   myWork[fu].operand1 = REG_FILE[srcReg1];
   if (srcReg2 !=  -1) 
     myWork[fu].operand2 = REG_FILE[srcReg2];
   myWork[fu].destReg = destReg;
   workAvail[fu] = TRUE; // Signal FU 
   
	
    if (DEBUG2) {
      printf("\n**********************************\n");
      printf("operand1: %d  operand2: %d    destReg: %d\n", myWork[fu].operand1, myWork[fu].operand2, myWork[fu].destReg);
      printf("\n**********************************\n");
    }


    scoreBoard[LSQindex + NUM_WAITBUFS][srcReg1] = FALSE;  
    updatePendingReads(srcReg1);
    if (srcReg2 != -1) {
      scoreBoard[LSQindex+NUM_WAITBUFS][srcReg2] = FALSE;
      updatePendingReads(srcReg2);
    }
   
   
    if (TRACE) 
      printf("\tActivating FU %s from LSQBuffer[%d] at time %5.2f\n", map(LSQ[LSQindex].fu), LSQindex, GetSimTime());
      
    LSQcount--;
    LSQ[LSQindex].free = TRUE;
    LSQhead = (LSQhead + 1) % MAX_NUM_LSQ_SLOTS;

  return;  // At most 1 MEM instruction dispatched in any cycle

    if (DEBUG2)
      printf("Freeing up LSQ entry %d\n", LSQindex);
    
  

 if (TRACE)
   printf("\tCould not find any MEM instruction to dipatch. Time: %5.2f\n", GetSimTime());
}
