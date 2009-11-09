/* ROM-Cache-MEM2.c - ROM Cache utilizing MEM2 on Wii
   by Mike Slegeir for Mupen64-GC
 ******************************************************
   Optimization: Store whatever blocks that don't fit
                   in MEM2 cache on the filesystem
                   under /tmp; keep an extra block for
                   asynchronous write-back
                 Create a L1 cache in MEM1
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include "../fileBrowser/fileBrowser.h"
#include "../gui/gui_GX-menu.h"
#include "../gc_memory/MEM2.h"
#include "../r4300/r4300.h"
#include "../gui/DEBUG.h"
#include "../gui/GUI.h"
#include "ROM-Cache.h"
#include "gczip.h"
#include "zlib.h"

#ifdef MENU_V2
void LoadingBar_showBar(float percent, const char* string);
#define PRINT DUMMY_print
#define SETLOADPROG DUMMY_setLoadProg
#define DRAWGUI DUMMY_draw
#else
#define PRINT GUI_print
#define SETLOADPROG GUI_setLoadProg
#define DRAWGUI GUI_draw
#endif

#define BLOCK_SIZE  (512*1024)
#define BLOCK_MASK  (BLOCK_SIZE-1)
#define OFFSET_MASK (0xFFFFFFFF-BLOCK_MASK)
#define BLOCK_SHIFT (19)	//only change ME and BLOCK_SIZE
#define MAX_ROMSIZE (64*1024*1024)
#define NUM_BLOCKS  (MAX_ROMSIZE/BLOCK_SIZE)
#define LOAD_SIZE   (32*1024)

static u32   ROMSize;
static int   ROMTooBig;
//static int   ROMCompressed;
//static int   ROMHeaderSize;
static char* ROMBlocks[NUM_BLOCKS];
static int   ROMBlocksLRU[NUM_BLOCKS];
static fileBrowser_file* ROMFile;
static char readBefore = 0;

extern void showLoadProgress(float);
extern void pauseAudio(void);
extern void resumeAudio(void);
extern BOOL hasLoadedROM;

#ifdef USE_ROM_CACHE_L1
static u8  L1[256*1024];
static u32 L1tag;
#endif

void DUMMY_print(char* string) { }
void DUMMY_setLoadProg(float percent) { }
void DUMMY_draw() { }

PKZIPHEADER pkzip;

void ROMCache_init(fileBrowser_file* f){
  readBefore = 0; //de-init byteswapping
	ROMSize = f->size;
	ROMTooBig = ROMSize > ROMCACHE_SIZE;

	romFile_seekFile(f, 0, FILE_BROWSER_SEEK_SET);	// Lets be nice and keep the file at 0.
#ifdef USE_ROM_CACHE_L1
	L1tag = -1;
#endif	
}

void ROMCache_deinit(){
	//we don't de-init the romFile here because it takes too much time to fopen/fseek/fread/fclose
}

void ROMCache_load_block(char* dst, u32 rom_offset){
  if((hasLoadedROM) && (!stop))
    pauseAudio();
	showLoadProgress( 1.0f );
	u32 offset = 0, bytes_read, loads_til_update = 0;
	romFile_seekFile(ROMFile, rom_offset, FILE_BROWSER_SEEK_SET);
	while(offset < BLOCK_SIZE){
		bytes_read = romFile_readFile(ROMFile, dst + offset, LOAD_SIZE);
		byte_swap(dst + offset, bytes_read);
		offset += bytes_read;
		
		if(!loads_til_update--){
//			showLoadProgress( (float)offset/BLOCK_SIZE );
			loads_til_update = 32;
		}
	}
//	showLoadProgress( 1.0f );
	if((hasLoadedROM) && (!stop))
	  resumeAudio();
}

void ROMCache_read(u32* dest, u32 offset, u32 length){
  if(ROMTooBig){
		u32 block = offset>>BLOCK_SHIFT;
		u32 length2 = length;
		u32 offset2 = offset&BLOCK_MASK;
		
		while(length2){
			if(!ROMBlocks[block]){
  		  // The block we're trying to read isn't in the cache
				// Find the Least Recently Used Block
				int i, max_i = 0, max_lru = 0;
				for(i=0; i<NUM_BLOCKS; ++i)
					if(ROMBlocks[i] && ROMBlocksLRU[i] > max_lru)
						max_i = i, max_lru = ROMBlocksLRU[i];
				ROMBlocks[block] = ROMBlocks[max_i]; // Take its place
        ROMCache_load_block(ROMBlocks[block], offset&OFFSET_MASK);
				ROMBlocks[max_i] = 0; // Evict the LRU block
			}
			
			// Set length to the length for this block
			if(length2 > BLOCK_SIZE - offset2)
				length = BLOCK_SIZE - offset2;
			else length = length2;
		
			// Increment LRU's; set this one to 0
			int i;
			for(i=0; i<NUM_BLOCKS; ++i) ++ROMBlocksLRU[i];
			ROMBlocksLRU[block] = 0;
			
			// Actually read for this block
			memcpy(dest, ROMBlocks[block] + offset2, length);
			
			// In case the read spans multiple blocks, increment state
			++block; length2 -= length; offset2 = 0; dest += length/4; offset += length;
		}
	} else {
#ifdef USE_ROM_CACHE_L1
		if(offset >> 18 == (offset+length-1) >> 18){
			// Only worry about using L1 cache if the read falls
			//   within only one block for the L1 for now
			if(offset >> 18 != L1tag){
				memcpy(L1, ROMCACHE_LO + (offset&(~0x3FFFF)), 256*1024);
				L1tag = offset >> 18;
			}
			memcpy(dest, L1 + (offset&0x3FFFF), length);
		} else
#endif
		{
			memcpy(dest, ROMCACHE_LO + offset, length);
		}
	}
}

int ROMCache_load(fileBrowser_file* f){
	char txt[128];
#ifndef MENU_V2
	GUI_clear();
	GUI_centerText(true);
#endif
	sprintf(txt, "Loading ROM %s into MEM2",ROMTooBig ? "partially" : "fully");
	PRINT(txt);

	ROMFile = f;
	romFile_seekFile(f, 0, FILE_BROWSER_SEEK_SET);
	
	u32 offset = 0,loads_til_update = 0;
	int bytes_read;
	u32 sizeToLoad = MIN(ROMCACHE_SIZE, ROMSize);
	while(offset < sizeToLoad){
		bytes_read = romFile_readFile(f, ROMCACHE_LO + offset, LOAD_SIZE);
		
		if(bytes_read < 0){		// Read fail!

			SETLOADPROG( -1.0f );
			return -1;
		}
		//initialize byteswapping if it isn't already
		if(!readBefore)
		{
 			init_byte_swap(*(unsigned int*)ROMCACHE_LO);
 			readBefore = 1;
		}
		//byteswap
		byte_swap(ROMCACHE_LO + offset, bytes_read);
		
		offset += bytes_read;
		
		if(!loads_til_update--){
			SETLOADPROG( (float)offset/sizeToLoad );
			DRAWGUI();
#ifdef MENU_V2
			LoadingBar_showBar((float)offset/sizeToLoad, txt);
#endif
			loads_til_update = 16;
		}
	}
	
	if(ROMTooBig){ // Set block pointers if we need to
		int i;
		for(i=0; i<ROMCACHE_SIZE/BLOCK_SIZE; ++i)
			ROMBlocks[i] = ROMCACHE_LO + i*BLOCK_SIZE;
		for(; i<ROMSize/BLOCK_SIZE; ++i)
			ROMBlocks[i] = 0;
		for(i=0; i<ROMSize/BLOCK_SIZE; ++i)
			ROMBlocksLRU[i] = i;
	}
	
	SETLOADPROG( -1.0f );
	return 0;
}



