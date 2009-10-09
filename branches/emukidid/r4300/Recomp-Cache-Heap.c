

#include <stdlib.h>
#include "r4300.h"
#include "Invalid_Code.h"
#include "Recomp-Cache.h"

#if 0
typedef struct _meta_node {
	unsigned int  addrStart;
	unsigned int  addrEnd;
	void*         memory;
	unsigned int  size;
	unsigned int* lru;
} CacheMetaNode;
#else
typedef struct _meta_node {
	unsigned int  blockNum;
	void*         memory;
	unsigned int  size;
	unsigned int* lru;
} CacheMetaNode;
#endif

static int cacheSize;

#define HEAP_CHILD1(i) ((i<<1)+1)
#define HEAP_CHILD2(i) ((i<<1)+2)
#define HEAP_PARENT(i) ((i-1)>>2)

#define INITIAL_HEAP_SIZE (64)
static unsigned int heapSize = 0;
static unsigned int maxHeapSize = 0;
static CacheMetaNode** cacheHeap = NULL;

static void heapSwap(int i, int j){
	CacheMetaNode* t = cacheHeap[i];
	cacheHeap[i] = cacheHeap[j];
	cacheHeap[j] = t;
}

static void heapUp(int i){
	// While the given element is out of order
	while(i && *(cacheHeap[i]->lru) < *(cacheHeap[HEAP_PARENT(i)]->lru)){
		// Swap the child with its parent
		heapSwap(i, HEAP_PARENT(i));
		// Consider its new position
		i = HEAP_PARENT(i);
	}
}

static void heapDown(int i){
	// While the given element is out of order
	while(1){
		// Check against the 1st child
		if(HEAP_CHILD1(i) < heapSize &&
		   *(cacheHeap[i]->lru) > *(cacheHeap[HEAP_CHILD1(i)]->lru)){
			heapSwap(i, HEAP_CHILD1(i));
			i = HEAP_CHILD1(i);
		} else if(HEAP_CHILD2(i) < heapSize &&
		          *(cacheHeap[i]->lru) > *(cacheHeap[HEAP_CHILD2(i)]->lru)){
			heapSwap(i, HEAP_CHILD2(i));
			i = HEAP_CHILD2(i);
		} else break;
	}
}

static void heapify(void){
	int i;
	for(i=1; i<heapSize; ++i) heapUp(i);
}

static void heapPush(CacheMetaNode* node){
	if(heapSize == maxHeapSize){
		maxHeapSize = 3*maxHeapSize/2 + 10;
		cacheHeap = realloc(cacheHeap, maxHeapSize*sizeof(void*));
	}
	// Simply add it to the end of the heap
	// No need to heapUp since its the most recently used
	cacheHeap[heapSize++] = node;
}

static CacheMetaNode* heapPop(void){
	heapSwap(0, --heapSize);
	heapDown(0);
	return cacheHeap[heapSize];
}


static void release(int minNeeded){
	// Frees alloc'ed blocks so that at least minNeeded bytes are available
	int toFree = minNeeded * 2; // Free 2x what is needed
	// Restore the heap properties to pop the LRU
	heapify();
	// Release nodes' memory until we've freed enough
	while(toFree > 0 || !cacheSize){
		// Pop the LRU to be freed
		CacheMetaNode* n = heapPop();
		// Free the code associated with n, and adjust the size accordingly
		free(n->memory);
		cacheSize -= n->size;
		toFree    -= n->size;
#if 1
		// Mark this block as invalid_code
		invalid_code_set(n->blockNum, 1);
		// Remove any pointers to this code
		// FIXME: Consider other equivalent addresses?
		blocks[n->blockNum]->code = NULL;
#else
		// Remove any pointers to this code
		// FIXME: Consider other equivalent addresses?
		PowerPC_block* block = blocks[n->addrStart>>12];
		// TODO: How do I do this exactly?
#endif
		free(n);
	}
}

void* RecompCache_Alloc(unsigned int size, unsigned int blockNum){
	CacheMetaNode* newBlock = malloc( sizeof(CacheMetaNode) );
	newBlock->blockNum = blockNum;
	newBlock->size = size;
	newBlock->lru = &(blocks[blockNum]->lru);
	
	if(cacheSize + size > RECOMP_CACHE_SIZE)
		// Free up at least enough space for it to fit
		release(cacheSize + size - RECOMP_CACHE_SIZE);
	
	// We have the necessary space for this alloc, so just call malloc
	cacheSize += size;
	newBlock->memory = malloc(size);
	// Add it to the heap
	heapPush(newBlock);
	// Make this block the LRU
	RecompCache_Update(blockNum);
	// Return the actual pointer
	return newBlock->memory;
}

void* RecompCache_Realloc(void* memory, unsigned int size){
	int i;
	CacheMetaNode* n = NULL;
	// Find the corresponding node
	for(i=heapSize-1; !n; --i)
		if(cacheHeap[i]->memory == memory)
			n = cacheHeap[i];
	// Make this block the LRU
	RecompCache_Update(n->blockNum);
	
	int neededSpace = size - n->size;
	
	if(cacheSize + neededSpace > RECOMP_CACHE_SIZE)
		// Free up at least enough space for it to fit
		release(cacheSize + neededSpace - RECOMP_CACHE_SIZE);
	
	// We have the necessary space for this alloc, so just call malloc
	cacheSize += neededSpace;
	n->memory = realloc(n->memory, size);
	n->size = size;
	
	// Return the actual pointer
	return n->memory;
}

void RecompCache_Free(unsigned int blockNum){
	int i;
	CacheMetaNode* n = NULL;
	// Find the corresponding node
	for(i=heapSize-1; !n && i>=0; --i)
		if(cacheHeap[i]->blockNum == blockNum)
			n = cacheHeap[i];
	if(!n) return;
	// Remove from the heap
	heapSwap(i, --heapSize);
	// Free n's memory
	free(n->memory);
	cacheSize -= n->size;
	free(n);
}

void RecompCache_Update(unsigned int blockNum){
	static unsigned int nextLRU = 0;
	blocks[blockNum]->lru = nextLRU++;
}

