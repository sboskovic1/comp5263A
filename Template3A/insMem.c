#include "global.h"


extern int TRACE;
extern unsigned insMEM[];

void loadPropgrsm();


void loadProgram() {
  int i;

  for (i=0; i < INSTR_MEM_SIZE; i++) 
      insMEM[i] =  0;  
  
  // PROGRAM  1
  // Accumulates the sum of an array of n consecutive memory words: Results in Register R7
  // The base address of the array is in Register R2
  // Initialize R8 with the number of iterations 
  /*  
         
  insMEM[0] = 0x10400000;       // LOADFP R0, (R2)       top: R0 = MEM[index];
  insMEM[1] = 0x15094000;       // INTADD  R8, R8, R9         n = n - 1;
  insMEM[2] = 0x04E03800;       // ADDFP R7, R7, R0           sum = sum + R0;
  insMEM[3] = 0x14431000;       // INTADD  R2, R2, R3         index = index + 1;
  insMEM[4] = 0x2500FFEC;       // BNEZ R8  -20               if (n != 0) goto top 
  insMEM[5] = 0x28000000;       //HALT 
  */


  // PROGRAM  2

  // Compute the dot product of two n-element vectors: Result in  Register R7
  // The base addresses of the two arrays are in Registers R2 and R4
  // Initialize R8 with the number of iterations n 
    
  /*            
  insMEM[0] = 0x15094000;       // INTADD  R8, R8, R9            top: n = n-1;
  insMEM[1] = 0x10400000;       // LOADFP R0, (R2)                    R0 = MEM[index1]
  insMEM[2] = 0x10800800;       // LOADFP R1, (R4)                    R1 = MEM[index2]
  insMEM[3] = 0x0C012800;       // MULFP R5, R0, R1                   R5 = R0 * R1
  insMEM[4] = 0x14431000;       // INTADD  R2, R2, R3                 indexSrc1 = indexSrc1 + 1
  insMEM[5] = 0x14832000;       // INTADD  R4, R4, R3                 indexSrc2 = indexSrc2 + 1 
  insMEM[6] = 0x04E53800;       // ADDFP R7, R7, R5                   R7 = R7 + R5
  insMEM[7] = 0x2500FFE0;       // BNEZ R8  -32                       if (n != 0) goto top
  insMEM[8] = 0x28000000;       //HALT 
  */
  
  // PROGRAM  3
  // Adds two vectors element-by-element and stores the result in a third array
  // The base addresses of the source arrays are in Registers R2 and R4
  // The base address  of the destination array is R6
  // Initialize R8 with the number of iterations
  
  
  insMEM[0] = 0x15094000;       // INTADD  R8, R8, R9            top: n = n-1;
  insMEM[1] = 0x10400000;       // LOADFP R0, (R2)                    R0 = MEM[R2]
  insMEM[2] = 0x10800800;       // LOADFP R1, (R4)                    R1 = MEM[R4]
  insMEM[3] = 0x04012800;       // ADDFP R5, R0, R1                   R5 = R0 + R1
  insMEM[4] = 0x14431000;       // INTADD  R2, R2, R3                 R2 = R2 + 1
  insMEM[5] = 0x14832000;       // INTADD  R4, R4, R3                 R4 = R4 + 1 
  insMEM[6] = 0x18C50000;       // SD R5, (R6)                        MEM[R6] = R5
  insMEM[7] = 0x14C33000;       // INTADD  R6, R6, R3                 R6 = R6 + 1
  insMEM[8] = 0x2500FFDC;       // BNEZ R8  -36                       if (n != 0) goto top
  insMEM[9] = 0x28000000;       //HALT 
  

}


