#include "global.h"

extern void   initializePhysicalMemory();
extern void   initializeCache();
extern void   initializeStatistics(int);
extern void   updatePendingReads(int);

extern int NUM_WAITBUFS, NUM_FU, NUM_COPIES, NUM_ITERATIONS, TRACE;
extern struct WaitBufEntry  insWaitBuffer[];
extern struct LSQEntry LSQ[];
extern unsigned REG_FILE[];
extern int pendingWrite[];
extern int scoreBoard[][NUM_REGISTERS];

extern unsigned  MEM[];
extern unsigned insMEM[];

extern unsigned PC;
extern int stallIF;
extern unsigned nextPC;
extern unsigned branchFlag;
extern unsigned INSTRUCTION;
extern int isFree[];
extern int resultReady[];
extern int resultData[];
extern int myWork[]; 
extern int workAvail[];

extern unsigned  PhysicalBaseAddress; // Array A will begin here
extern void loadProgram();

void loadRegFile();

void loadRegFile() {  
  REG_FILE[0] = 0;  
  REG_FILE[1] = 0;
  REG_FILE[2] = PhysicalBaseAddress;  
  REG_FILE[3] = sizeof(unsigned);    
  REG_FILE[4] = PhysicalBaseAddress + 128 * sizeof(unsigned);  
  REG_FILE[5] = 0;
  REG_FILE[6] = PhysicalBaseAddress + 256 * sizeof(unsigned);  
  REG_FILE[7] = 0;  
  REG_FILE[8] = NUM_ITERATIONS;
  REG_FILE[9] = -1;
  }

void reset() {
  int i, j;

  initializePhysicalMemory();
  initializeCache();
  for (i=0; i < MAX_NUM_THREADS; i++)
    initializeStatistics(i);

  // Initialize Wait Buffer
  for (i=0; i < MAX_NUM_WAITBUFS; i++) {
    insWaitBuffer[i].fu = -1;
    insWaitBuffer[i].op1Ready = FALSE;
    insWaitBuffer[i].op2Ready = FALSE;
    insWaitBuffer[i].srcReg1 = -1;
    insWaitBuffer[i].srcReg2 = -1;
    insWaitBuffer[i].free = TRUE;
  }

  for (i=0; i < MAX_NUM_LSQ_SLOTS; i++) {
    LSQ[i].fu = -1;
    LSQ[i].op1Ready = FALSE;
    LSQ[i].op2Ready = FALSE;
    LSQ[i].srcReg1 = -1;
    LSQ[i].srcReg2 = -1;
  }

  // Initialize all Function Units
  // There are NUM_FU types of functional units with  NUM_COPIES units of each type.
  for (i=0; i < MAX_NUM_FU * MAX_NUM_COPIES; i++) {
    isFree[i] = TRUE;
    resultReady[i] = FALSE;
    resultData[i] = -1;
    workAvail[i] = FALSE;
}

  // Reset  Registers
  for (i=0; i < NUM_REGISTERS; i++) {
    REG_FILE[i] = 0;
  }

  // Reset Scoreboard
   for (i=0; i < MAX_NUM_WAITBUFS; i++)
     for (j=0; j < NUM_REGISTERS; j++)
      scoreBoard[i][j] = FALSE;

   // Reset Pending Writes and Reads
   for (i=0; i < NUM_REGISTERS; i++) {
       pendingWrite[i] = 0; // No writer
       updatePendingReads(i);
   }


  branchFlag = FALSE;
  PC = 0;
  nextPC = 0;
  stallIF = FALSE;
  INSTRUCTION = NOP;
  
  loadRegFile();
  loadProgram();
}

void showRegFile() {
  int i, j;
  for (j=0; j < 2; j++) {
    for (i=0; i < NUM_REGISTERS/2; i++) 
    printf("REG[%d]: %d\t", j*NUM_REGISTERS/2+ i, REG_FILE[j*NUM_REGISTERS/2+i]);
  printf("\n");
  }
}


char * map(int fu) {
  int opcode;

  opcode = fu % NUM_FU;
  switch(opcode) {
  case 1:     return("ADDFP");
  case 2:     return("SUBFP");
  case 3:     return("MULFP");
  case 4:     return("LOADFP");
  case 5:     return("INTADD");
  case 6:     return("STOREFP");
    //  case 7:     return("COPYFP");
  default:    return("---");
  }  
}

char *bool(int signal) {
  return((signal == TRUE) ? "TRUE" : "FALSE");
}


void showinsWaitBuffer(int i) {
  if (insWaitBuffer[i].free == TRUE)
    return;
  printf("\tinsWaitBuffer[%d]\tFREE: %s FU: %s Destination Register: %d\n", i, bool(insWaitBuffer[i].free),  map(insWaitBuffer[i].fu), insWaitBuffer[i].destReg);

  printf("\t\top1Ready: %s srcReg1: %d\n", bool(insWaitBuffer[i].op1Ready),  insWaitBuffer[i].srcReg1);
  printf("\t\top2Ready: %s srcReg2: %d\n", bool(insWaitBuffer[i].op2Ready),  insWaitBuffer[i].srcReg2);

}
