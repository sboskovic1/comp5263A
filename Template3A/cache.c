#include "global.h"

// Cache Variables
struct cacheTagBlock    CACHE_TAG_ARRAY[MAX_CACHE_SIZE_SETS][MAX_NUM_WAYS];  
struct cacheDataBlock  CACHE_DATA_ARRAY[MAX_CACHE_SIZE_SETS][MAX_NUM_WAYS];  
extern int NUM_WAY_BITS;
extern int NUM_WAYS;  // Cache Associativity
extern int  CACHE_SIZE_BITS;
extern int CACHE_SIZE_BYTES;   // Size of Cache in Bytes
extern int CACHE_SIZE_SETS; // NumSets in Cache
extern int POLICY;
extern int usage[][MAX_NUM_WAYS]; // Used to implement pseuso-LRU eviction policy


// Statistics Variables
extern int cache_hits[], cache_misses[];
extern int cache_reads[], cache_writes[];
extern int cache_writebacks[];

extern unsigned * PhysicalBaseAddress;  // Starting MEM address for data

extern  int TRACE;
extern int NUM_THREADS;

extern double drand48();
extern void writeBlock(unsigned *, int, int, int);
extern void loadBlock(unsigned *, int, int, int);
void cacheWrite(int, int, unsigned *, unsigned *, unsigned *);

int getVictimRANDOM(int index) {
  return drand48() * NUM_WAYS;
}

/* ***********************************  PLRU ***************************************  */

int getVictimPLRU(int index) {
  int i, next, victim;

    next= 1;
    for (i=0; i < NUM_WAY_BITS; i++) {
    if (usage[index][next] == 0)
      next = 2 * next + 1;
    else
      next = 2 * next;
  }
    victim = next % NUM_WAYS;
    if (DEBUG)
      printf("Choosing victim for Index: %d Way: %d\n", index, victim);
    return victim;
}

// Helper Function
int getBit(int way, int bitnum) {
  int mask = 0x1 << bitnum;
  return((way & mask)  >> bitnum);
}

// Update the access path of a reference block
void updateUsage(int index,int way) {
  int next, i, temp;

  for (next = 1, i = NUM_WAY_BITS-1; i >= 0; i--) { 
  temp = getBit(way, i);
  usage[index][next] =  temp;
  if (DEBUG)
    printf("usage[%d][%d]: %d\n", index, next, usage[index][next]);
  next = 2 *next + temp;
  }
}

/* ***********************************  END PLRU ***************************************  */

int getVictim(int index) {
  if (POLICY == RANDOM)
    return(getVictimRANDOM(index));
  else
    return(getVictimPLRU(index));
}

unsigned  getWritebackAddress(int cacheIndex, int way) {
  // Return the writeback memory address of specified cache block
  int cacheTag = CACHE_TAG_ARRAY[cacheIndex][way].TAG;
  return ( ( (unsigned long) cacheTag * CACHE_SIZE_SETS) + cacheIndex) << BLK_SIZE_BITS;  
}

void flushCache() {
  int cacheIndex, way;
  unsigned *writebackAddress;

  for (cacheIndex=0; cacheIndex < CACHE_SIZE_SETS; cacheIndex++) {
    for (way=0; way < NUM_WAYS; way++) {
      if (CACHE_TAG_ARRAY[cacheIndex][way].V && CACHE_TAG_ARRAY[cacheIndex][way].D) {
	writebackAddress = (unsigned *) (unsigned long) getWritebackAddress(cacheIndex, way);
	if (TRACE)
	  printf("Flushing Cache Block Index %d  Way: %d to Memory  Address %x  Time: %5.2f\n", cacheIndex, way, writebackAddress, GetSimTime());
	writeBlock(writebackAddress, cacheIndex, way, BLK_SIZE_INTS);
       }
    }
  }
}




int  handleCacheMiss(int thread_id, unsigned long  address, int cacheIndex) {
    int way;
    int myCacheTag;
    unsigned *writebackAddress, *loadAddress;

    myCacheTag = (address >> BLK_SIZE_BITS) / CACHE_SIZE_SETS;
    way = getVictim(cacheIndex);
    cache_misses[thread_id]++;

     // If victim block is DIRTY writeback to memory
     if (CACHE_TAG_ARRAY[cacheIndex][way].V  &&  CACHE_TAG_ARRAY[cacheIndex][way].D) {
	writebackAddress = (unsigned *) (unsigned long) getWritebackAddress(cacheIndex, way);  // Get memeory address of dirty victim
	if (TRACE)
	  printf("Writeback Block Index %d Way %d to Memory Address %p Time %5.2f\n", cacheIndex, way, writebackAddress, GetSimTime());
	writeBlock(writebackAddress, cacheIndex, way, BLK_SIZE_INTS);
	CACHE_TAG_ARRAY[cacheIndex][way].D = FALSE;
	if (TRACE) 
	  printf("Cache Writeback completed at time %5.2f\n", GetSimTime());
	cache_writebacks[thread_id]++;
     }
     // Read missed block into cache
     loadAddress = (unsigned *) (address & (-1 << BLK_SIZE_BITS) );  // Address must be aligned with block boundary
     loadBlock(loadAddress, cacheIndex, way, BLK_SIZE_INTS);
     CACHE_TAG_ARRAY[cacheIndex][way].TAG  = myCacheTag;
     CACHE_TAG_ARRAY[cacheIndex][way].V  = TRUE;
     
     ProcessDelay(CACHE_INSERT_TIME);  // Cycle to add fetched block to cache and update metadata (tag and status) 
     if (TRACE) 
       printf("\tBlock address %x now in cache:  Index %d  Way %d Time %5.2f (MM)\n", address, cacheIndex, way, GetSimTime());
     cache_hits[thread_id]++;
     return(way);
}



void   LookupCache(int thread_id, unsigned long address, int *pcacheIndex, int *pway) {
    unsigned cacheIndex;
    int way;
    int myCacheTag;
    int cacheV, cacheTag;

    cacheIndex = (address >> BLK_SIZE_BITS) % CACHE_SIZE_SETS;
    myCacheTag = (address >> BLK_SIZE_BITS) / CACHE_SIZE_SETS;

    *pcacheIndex = cacheIndex;   // Return the cache Index of block

  for (way=0; way < NUM_WAYS; way++) {
    cacheV = CACHE_TAG_ARRAY[cacheIndex][way].V;
    cacheTag = CACHE_TAG_ARRAY[cacheIndex][way].TAG;
    if ((cacheV == TRUE)  && (cacheTag == myCacheTag) ) {
	cache_hits[thread_id]++;
	if (TRACE) 
	  printf("\tCache HIT for Address: %x Cache Index: %d at time %5.2f (MM)\n", address, cacheIndex, GetSimTime());
	ProcessDelay(CACHE_TAGCHECK_TIME);
	
	*pway = way;  // Return the way of the block
	updateUsage(cacheIndex, way);  // Update the Pseudo-LRU data structure for the access
	return;;
      }
  }
  // Cache Miss: Not found in any way of the cache
  ProcessDelay(CACHE_TAGCHECK_TIME);
  if (TRACE) 
      printf("\tCache MISS for Address: %x  Cache Index: %d at time %5.2f (MM)\n", address, cacheIndex, GetSimTime());
  way = handleCacheMiss(thread_id, address, cacheIndex); // Get requested block into the cache
  updateUsage(cacheIndex, way);  // Update the Pseudo-LRU data structure for the access
  *pway = way;
}

void cacheWrite(int thread_id, int value, unsigned * memAddress, unsigned *AcacheIndex, unsigned *AcacheWay) {
  int cacheIndex, way;
  int byteOffset;
  unsigned long address = (unsigned long) memAddress;

  if (TRACE)   
    printf("\nWRITE Request at time %5.2f: Thread %d Value %d Address: %p\n", GetSimTime(), thread_id, value, memAddress);

  
  // Get cache index for request. LookupCache will handle a cache miss  before returning.
  //  LookupCache(thread_id, memAddress, &cacheIndex, &way);
  LookupCache(thread_id, address, &cacheIndex, &way);
  
  // Write the word from the cache block at cacheIndex
  byteOffset = address & ((1 << BLK_SIZE_BITS) - 1);
  CACHE_DATA_ARRAY[cacheIndex][way].BLKDATA[byteOffset/sizeof(int)] = value;
  CACHE_TAG_ARRAY[cacheIndex][way].D = TRUE;

  cache_writes[thread_id]++; 
  if (TRACE)
    printf("\tWRITE completed: Time %5.2f: Value %d memAddress: %p (MM)\n", GetSimTime(),  value, memAddress);

 *AcacheIndex = cacheIndex;
  *AcacheWay = way;
}


int cacheRead(int thread_id,  unsigned * memAddress, unsigned *AcacheIndex, unsigned *AcacheWay) {
   int cacheIndex, way;
  int byteOffset;
    int value;
    unsigned long address = (unsigned long) memAddress;

  if (TRACE)   
    printf("\tREAD Request at time %5.2f memAddress: %p (MM)\n", GetSimTime(), memAddress);

  //  LookupCache(thread_id, memAddress, &cacheIndex, &way);
  LookupCache(thread_id, address, &cacheIndex, &way);

  // Returns with "cacheIndex" and "way" of the requested block

  byteOffset = address & ((1 << BLK_SIZE_BITS) - 1);
  value = CACHE_DATA_ARRAY[cacheIndex][way].BLKDATA[byteOffset/sizeof(int)]; // Always read word-sized (int) data
  cache_reads[thread_id]++;

   if (TRACE)
     printf("\tREAD completed: Time %5.2f: Value %d address: %p byteOffset: %d (MM)\n", GetSimTime(), value, memAddress, byteOffset);
  *AcacheIndex = cacheIndex;
  *AcacheWay = way;
  return value;
}











