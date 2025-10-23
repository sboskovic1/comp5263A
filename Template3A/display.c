#include "global.h"
extern int NUM_WAITBUFS, NUM_FU, NUM_COPIES, NUM_ITERATIONS;
extern int TRACE;

extern unsigned REG_FILE[];
extern int pendingWrite[];
extern double timeInsCompleted, timeInsRetired;
extern unsigned INSTRUCTION;
extern struct WaitBufEntry insWaitBuffer[];
extern int isHALT;
extern char * map(int);


extern   int isFree[MAX_NUM_FU * MAX_NUM_COPIES];
extern  int resultReady[MAX_NUM_FU * MAX_NUM_COPIES];
extern  int workAvail[MAX_NUM_FU * MAX_NUM_COPIES];
extern int LSQcount;

void showWaitBuffer(int);
void do_display();

void displaystage()
{
  int job_num;


  job_num = ActivityArgSize(ME) - 1;
  while(1){	
       if (TRACE)
       printf("In DISPLAY  Stage at time %2.0f\n\n", GetSimTime());
   
    do_display();
    ProcessDelay(1.000);
  }
}

void showWaitBuffer(int index){
  
  int free = insWaitBuffer[index].free;
  int fu = insWaitBuffer[index].fu;
  int  op1Ready =  insWaitBuffer[index].op1Ready;
  int  op2Ready =  insWaitBuffer[index].op2Ready;
 unsigned  srcReg1 =  insWaitBuffer[index].srcReg1;
 unsigned  srcReg2 =  insWaitBuffer[index].srcReg2;
 unsigned  destReg =  insWaitBuffer[index].destReg;


   printf("WAITBUFFER[%d]: free: %d FU: %s srcReg1: %d srcReg2: %d destReg: %d  op1Ready: %d op2Ready: %d\n", index, free, map(fu), srcReg1, srcReg2, destReg, op1Ready, op2Ready);

}

void do_display(){
 
  static int endExecutionDetected = FALSE;
  static int endRetirementDetected = FALSE;
  int i, j, allRSFree;
  int fu;

  if ( (endExecutionDetected == FALSE) && (isHALT == TRUE)) {
    endExecutionDetected = TRUE;
  timeInsCompleted = GetSimTime();
  }

  allRSFree = TRUE;

  for (i=0; i < NUM_WAITBUFS; i++) {
    if (insWaitBuffer[i].free == FALSE) {
      allRSFree = FALSE;
      break;
    } 
  }

for (i=0; i < NUM_WAITBUFS; i++) {
    if (insWaitBuffer[i].free == FALSE) {
      allRSFree = FALSE;
      break;
    } 
  }
  
  //new
  for (fu = 0; fu < MAX_NUM_FU*MAX_NUM_COPIES; fu++) {
    if ( !isFree[fu] || resultReady[fu] || workAvail[fu] )
      return;
}
  
  if ( (endExecutionDetected == TRUE) && (endRetirementDetected == FALSE) && (allRSFree == TRUE) && (LSQcount == 0)) {
    endRetirementDetected = TRUE;
    timeInsRetired  = GetSimTime();
  }

 /* Use this to print out the different system state */


    if (DEBUG)  
    {
      //      printf("************************\n");
      //for (i=0; i < NUM_REGISTERS; i++)
      //	printf("REG[%d] : %d  pendingWrite[%d]: %d\n", i, REG_FILE[i], i, pendingWrite[i]); 
      printf("************************\n");
        for (i=0; i < NUM_WAITBUFS; i++)
      	showWaitBuffer(i);
      printf("************************\n");
    }
    
}



/*******************************************************************************/


