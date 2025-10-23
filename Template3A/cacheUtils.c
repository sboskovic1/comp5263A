#include "global.h"

extern int NUM_WAY_BITS;
extern int CACHE_SIZE_BITS;

extern int NUM_THREADS;
extern int POLICY;


extern int NUM_WAYS;
extern int CACHE_SIZE_BYTES;   // Size of Cache in Bytes
extern int CACHE_SIZE_SETS; // NumSets in Cache


extern struct cacheTagBlock  CACHE_TAG_ARRAY[][MAX_NUM_WAYS];
extern struct cacheDataBlock  CACHE_DATA_ARRAY[][MAX_NUM_WAYS]; 

extern int cache_hits[], cache_misses[];
extern int cache_reads[], cache_writes[];
extern int cache_writebacks[];
extern unsigned * PhysicalBaseAddress;

extern unsigned PHYMEM[];
extern int usage[][MAX_NUM_WAYS];

void printStatistics(int thread_id) {
  printf("Thread %d ending  at %5.2f\n",thread_id, GetSimTime());
  printf("Cache Hits: %d  Cache Misses: %d Cache Reads: %d Cache Writes: %d Cache Writebacks: %d \n", cache_hits[thread_id], cache_misses[thread_id], cache_reads[thread_id], cache_writes[thread_id], cache_writebacks[thread_id]);
}

void initializeStatistics(int id) {
    cache_hits[id] = 0;
    cache_misses[id] = 0;
    cache_writes[id] = 0;
    cache_reads[id] = 0;
    cache_writebacks[id] = 0;
}

void initializePhysicalMemory() {
  int i;

  PhysicalBaseAddress  = ((unsigned *) ( ( (long) PHYMEM)  & (-1 << (CACHE_SIZE_BITS - NUM_WAY_BITS) ))) + (1 << (CACHE_SIZE_BITS-NUM_WAY_BITS-2) );
  printf("Base address of Physical Memory: %p\n", PHYMEM); 
  printf("Base Physical Address allocated to  arrays: %p\n\n", (unsigned *) PhysicalBaseAddress); 

    for (i=0; i < MEM_SIZE_INTS; i++) {
       PHYMEM[i] = (unsigned) -1;  // Initialize all words of physical memory to 0xFFFFFFFF
    } 

    for (i=0; i < 1024; i++) {
      *(PhysicalBaseAddress + i) = (unsigned) i+1;  // Initialize array locations from 1, 2,3, ......
    }
    printf("Initialized Array memory Locations to 1, 2, 3, ...\n");
}


void initializeCache() {
  int i, j, k;
  
  for (i=0; i < CACHE_SIZE_SETS; i++) {
    for (j=0; j < NUM_WAYS; j++) {
      CACHE_TAG_ARRAY[i][j].V = FALSE;
      usage[i][j] = 1;
      for (k=0; k < BLK_SIZE_INTS; k++)
	CACHE_DATA_ARRAY[i][j].BLKDATA[k] = -1;
    }
  }
  for (i=0; i < NUM_THREADS; i++) {
    cache_hits[i] = 0;
    cache_misses[i]= 0;
  }
  printf("Initialized Cache ....\n");
}


void writeBlock(int  *phyAddress, int cacheIndex, int way, int numIntsInBlock) {
  int i;

  ProcessDelay(MEM_CYCLE_TIME);
  for (i=0; i < numIntsInBlock; i++) {
    *phyAddress = CACHE_DATA_ARRAY[cacheIndex][way].BLKDATA[i];
    if (DEBUG)
      printf("WRITE ADDRESS %p VALUE: %d\n", phyAddress,CACHE_DATA_ARRAY[cacheIndex][way].BLKDATA[i]);
    phyAddress++;
  }
  if (DEBUG)
    printf("\nDirty Block at Cache Index %d written  to physical address: %p at time %5.2f\n", cacheIndex, phyAddress-numIntsInBlock, GetSimTime());
}


void loadBlock(int *phyAddress, int cacheIndex, int way, int numIntsInBlock) {
  int i;

  ProcessDelay(MEM_CYCLE_TIME);
  for (i=0; i < numIntsInBlock; i++) {
    CACHE_DATA_ARRAY[cacheIndex][way].BLKDATA[i] = *phyAddress;
      if (DEBUG)
      printf("CACHE LOAD %p VALUE: %d\n", phyAddress, *phyAddress);
    phyAddress++;
  }
  if (DEBUG) 
    printf("Block at Cache Index %d loaded from physical address: %p at time %5.2f\n", cacheIndex, phyAddress-numIntsInBlock, GetSimTime());
}



void displayVector(char *name, int *p, int count) {
   int i, j;
   for (i=0; i < SIZE/8; i++){
     for (j=0; j < 8; j++)
       printf("%s[%d]: %d\t", name, i*8+j, p[i*8 +j]);
     printf("\n");
   }
 }




