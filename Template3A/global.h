#include <math.h>
#include <stdio.h>
#include "sim.h"

#define epsilon 0.001
#define MAX_SIMULATION_TIME 200000.0  // 20.0 (trace1), 45.0 (trace2), 60000 (1024 iterations)
#define TRUE 1
#define FALSE 0
#define DEBUG FALSE
#define DEBUG2  FALSE

#define NOP 0
#define ADDFP 1
#define SUBFP 2
#define MULFP 3
#define LOADFP 4
#define INTADD 5
#define STOREFP 6
#define BRANCH 8
#define BNEZ 9
#define HALT 10

#define ADDFP_CYCLES 256 //256 
#define SUBFP_CYCLES  4
#define MULFP_CYCLES  256 //  256 
#define LOADFP_CYCLES   0 // 0
#define INTADD_CYCLES 1
#define STOREFP_CYCLES  2 // 2
#define COPYFP_CYCLES 1

#define MAX_NUM_WAITBUFS 256
#define MAX_NUM_LSQ_SLOTS 4
#define MAX_NUM_FU 32
#define MAX_NUM_COPIES 16
#define NUM_REGISTERS 16

#define MEM_SIZE  8192  
#define INSTR_MEM_SIZE 1024

/*
#define BASE_ARRAY 0
#define BASE_ARRAY_SRC1 0
#define BASE_ARRAY_SRC2 128 
#define BASE_ARRAY_DEST 256 
*/

#define MAX_NUM_ITERATIONS 1024
struct WaitBufEntry {
  int free;
  int fu;
  int  op1Ready, op2Ready;
  unsigned  srcReg1, srcReg2, destReg;
};

struct LSQEntry {
  int free;
  int fu;
  int  op1Ready, op2Ready;
  unsigned  srcReg1, srcReg2, destReg;
};


struct workEntry {
  unsigned  operand1, operand2, destReg;
};


struct resultEntry {
  unsigned  result, destReg;
};



//  Cache and Memory definitions

#define PLRU 1
#define RANDOM 0

#define SIZE 128  // Data size for all three programs
#define MAX_NUM_THREADS 1


// Memory Parameters
#define MEM_SIZE_BITS 24
#define MEM_SIZE_BYTES  (0x1 << MEM_SIZE_BITS)
#define MEM_SIZE_INTS (MEM_SIZE_BYTES/sizeof(int)) // Size in units of ints
#define MAX_DATA_SIZE  MEM_SIZE_INTS


// Cache Parameters
#define BLK_SIZE_BITS  5
#define  BLK_SIZE_BYTES   (0x1 << BLK_SIZE_BITS)  // Size in Bytes
#define  BLK_SIZE_INTS    (BLK_SIZE_BYTES/sizeof(int))  // Size in units of "ints"

#define MAX_NUM_WAYS  8  // Maximum Cache Associativity
#define MAX_CACHE_SIZE_BYTES 4096   // 4KB Maximum Size of Cache in Bytes
#define MAX_CACHE_SIZE_SETS MAX_CACHE_SIZE_BYTES/BLK_SIZE_BYTES // Maximum NumSets in Cache


// Timing Parameters
#define CLOCK_CYCLE 1.0
#define CACHE_LOOKUP_TIME 1.0
#define CACHE_TAGCHECK_TIME 1.0
#define CACHE_INSERT_TIME 1.0
#define MEM_CYCLE_TIME  128.0


struct cacheTagBlock {
  int V;
  int D;
  int TAG;
};

struct cacheDataBlock {
  int  BLKDATA[BLK_SIZE_INTS]; // Treating block as array of integers rather than bytes
};

