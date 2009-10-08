/* ROM-Cache.c - This is how the ROM should be accessed, this way the ROM doesn't waste RAM
   by Mike Slegeir for Mupen64-GC
 */

#include <ogc/arqueue.h>
#include <gccore.h>
#include "../gc_memory/ARAM.h"
#include "ROM-Cache.h"
#include "../fileBrowser/fileBrowser.h"

#ifdef USE_GUI
#include "../gui/GUI.h"
#define PRINT GUI_print
#else
#define PRINT printf
#endif
#include "../gui/DEBUG.h"


#define BLOCK_MASK  (BLOCK_SIZE-1)
#define OFFSET_MASK (0xFFFFFFFF-BLOCK_MASK)
#define BLOCK_SHIFT (20)	//only change ME and BLOCK_SIZE in gc_memory/aram.h
#define MAX_ROMSIZE (64*1024*1024)
#define NUM_BLOCKS  (MAX_ROMSIZE/BLOCK_SIZE)

static char ROM_too_big;
static char* ROM, * ROM_blocks[NUM_BLOCKS];
static u32 ROM_size;
static fileBrowser_file* ROM_file;


#ifdef USE_ROM_CACHE_L1
static u8  L1[256*1024];
static u32 L1tag;
#endif

static ARQRequest ARQ_request;
void showLoadProgress(float progress);

void ROMCache_init(u32 romSize){
	ARQ_Reset();
	ARQ_Init();
	ROM_too_big = romSize > (ARAM_block_available_contiguous() * BLOCK_SIZE);
	ROM_size = romSize;
#ifdef USE_ROM_CACHE_L1
	L1tag = -1;
#endif
	
	//romFile_init( romFile_topLevel );
}

void ROMCache_deinit(){
	if(ROM_too_big){
		int i;
		for(i=0; i<NUM_BLOCKS; ++i)
			if(ROM_blocks[i])
				ARAM_block_free(&ROM_blocks[i]);
	} else
		if(ROM) ARAM_block_free_contiguous(&ROM, ROM_size / BLOCK_SIZE);
	//romFile_deinit( romFile_topLevel );
}

static void inline ROMCache_load_block(char* block, int rom_offset){
	romFile_seekFile(ROM_file, rom_offset, FILE_BROWSER_SEEK_SET);
	int bytes_read, offset=0, bytes_to_read=ARQ_GetChunkSize();
	char* buffer = memalign(32, bytes_to_read);
	int loads_til_update = 0;
	do {
		bytes_read = romFile_readFile(ROM_file, buffer, bytes_to_read);
		byte_swap(buffer, bytes_read);
		DCFlushRange(buffer, bytes_read);
		ARQ_PostRequest(&ARQ_request, 0x10AD, ARQ_MRAMTOARAM, ARQ_PRIO_HI,
		                block + offset, buffer, bytes_read);
		offset += bytes_read;
		
		if(!loads_til_update--){
			showLoadProgress( (float)offset/BLOCK_SIZE );
			loads_til_update = 16;
		}
		
	} while(offset != BLOCK_SIZE && bytes_read == bytes_to_read);
	free(buffer);
	showLoadProgress(1.0f);
}

//handles all alignment
void ARAM_ReadFromBlock(char *block,int startOffset, int bytes, char *dest)
{
  int originalStartOffset = startOffset;
  int originalBytes = bytes;
  
  if(startOffset%32 !=0)  //misaligned startoffset
  {
    startOffset -= startOffset % 32;
    bytes+=(originalStartOffset%32);  //adjust for the extra startOffset now
  }
  
  if(bytes%32 !=0)  //adjust again if misaligned size
    bytes += 32 - (bytes%32);
    
  char* buffer = memalign(32,bytes);
  
  ARQ_PostRequest(&ARQ_request, 0x2EAD, AR_ARAMTOMRAM, ARQ_PRIO_LO,
			                block + startOffset, buffer, bytes);
	DCInvalidateRange(buffer, bytes);
	memcpy(dest, buffer+(originalStartOffset%32), originalBytes);
	
	free(buffer);
}

void ROMCache_read(u32* ram_dest, u32 rom_offset, u32 length){

	if(ROM_too_big){ // The whole ROM isn't in ARAM, we might have to move blocks in/out
		u32 length2 = length;
		u32 offset2 = rom_offset&BLOCK_MASK;
		char *dest = (char*)ram_dest;
		while(length2){
  		char* block = ROM_blocks[rom_offset>>BLOCK_SHIFT];
  		if(!block){ // This block is not alloced
  			if(!ARAM_block_available())
  				ARAM_block_free(ARAM_block_LRU('R'));
  			block = ARAM_block_alloc(&ROM_blocks[rom_offset>>BLOCK_SHIFT], 'R');
  			ROMCache_load_block(block, rom_offset&OFFSET_MASK);
  		}
			
			// Set length to the length for this block
			if(length2 > BLOCK_SIZE - offset2)
				length = BLOCK_SIZE - offset2;
			else length = length2;
					
			// Actually read for this block
			ARAM_ReadFromBlock(block,offset2,length,dest);
			ARAM_block_update_LRU(&block);
			// In case the read spans multiple blocks, increment state
			length2 -= length; offset2 = 0; dest += length; rom_offset += length;
		}
	  
	} else { // The entire ROM is in ARAM contiguously
  	//just read
  	ARAM_ReadFromBlock(ROM,rom_offset,length,(char*)ram_dest);
	}

}

void ROMCache_load(fileBrowser_file* file, int byteSwap){
	char txt[64];
	
	if(byteSwap == BYTE_SWAP_BAD) return;
	ROM_byte_swap = byteSwap;
	if(byteSwap == BYTE_SWAP_BYTE) PRINT("Byte swapped\n");
	else if(byteSwap == BYTE_SWAP_HALF) PRINT("Halfword swapped\n");
	
	/*
	sprintf(txt, "Loading ROM into ARAM %s\n", ROM_too_big ? "(ROM_too_big)" : "");
	PRINT(txt);
	if(ROM_too_big) sprintf(txt, "%d blocks available\n", ARAM_block_available());
	else            sprintf(txt, "%d contiguous blocks available\n", ARAM_block_available_contiguous());
	PRINT(txt);
	*/
	
	GUI_clear();
	GUI_centerText(true);
	sprintf(txt, "Loading ROM %s into ARAM.\n Please be patient...\n", ROM_too_big ? "partially" : "fully");
	PRINT(txt);
	
	ROM_file = file;
	romFile_seekFile(ROM_file, 0, FILE_BROWSER_SEEK_SET);

	int bytes_to_read = ARQ_GetChunkSize();
	int* buffer = memalign(32, bytes_to_read);
	if(ROM_too_big){ // We can't load the entire ROM
		int i, block, available = ARAM_block_available();
		for(i=0; i<available; ++i){
			block = ARAM_block_alloc(&ROM_blocks[i], 'R');
			//sprintf(txt, "ROM_blocks[%d] = 0x%08x\n", i, block);
			//PRINT(txt);
			int bytes_read, offset=0;
			int loads_til_update = 0;
			do {
				bytes_read = romFile_readFile(ROM_file, buffer, bytes_to_read);
				byte_swap(buffer, bytes_read);
				DCFlushRange(buffer, bytes_read);
				ARQ_PostRequest(&ARQ_request, 0x10AD, AR_MRAMTOARAM, ARQ_PRIO_HI,
				                block + offset, buffer, bytes_read);
				offset += bytes_read;
				
				if(!loads_til_update--){
					GUI_setLoadProg((float)i/available + (float)offset/(available*BLOCK_SIZE));
					GUI_draw();
					loads_til_update = 32;
				}
				
			} while(offset != BLOCK_SIZE);
			GUI_setLoadProg( -1.0f );
		}
	} else {
		ARAM_block_alloc_contiguous(&ROM, 'R', ROM_size / BLOCK_SIZE);
		//sprintf(txt, "ROM = 0x%08x using %d blocks\n", ROM, ROM_size/BLOCK_SIZE);
		//PRINT(txt);
		int bytes_read, offset=0;
		int loads_til_update = 0;
		do {
			bytes_read = romFile_readFile(ROM_file, buffer, bytes_to_read);
			byte_swap(buffer, bytes_read);
			DCFlushRange(buffer, bytes_read);
			ARQ_PostRequest(&ARQ_request, 0x10AD, AR_MRAMTOARAM, ARQ_PRIO_HI,
			                ROM + offset, buffer, bytes_read);
			offset += bytes_read;
			
			if(!loads_til_update--){
				GUI_setLoadProg( (float)offset/ROM_size );
				GUI_draw();
				loads_til_update = 32;
			}
			
		} while(bytes_read == bytes_to_read && offset != ROM_size);
		GUI_setLoadProg( -1.0f );
	}
	free(buffer);
}




