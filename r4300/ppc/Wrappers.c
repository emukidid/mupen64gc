/* Wrappers.c - Dynarec trampoline and other functions to simplify access from dynarec
   by Mike Slegeir for Mupen64-GC
 */

#include <stdlib.h>
#include "../../gui/DEBUG.h"
#include "../Invalid_Code.h"
#include "../../gc_memory/memory.h"
#include "../interupt.h"
#include "../r4300.h"
#include "Wrappers.h"

extern int stop;
extern void (*interp_ops[64])(void);
inline unsigned long update_invalid_addr(unsigned long addr);


void dynarec(unsigned int address){
	// TODO: Set up registers (reg in r13, stack pointer, link register, etc)
	while(!stop){
		PowerPC_block* dst_block = blocks[address>>12];
		unsigned long paddr = update_invalid_addr(address);
		if(!paddr) return;
		
		// Check for interrupts
		update_count();
		if(next_interupt <= Count) gen_interupt();
		
		sprintf(txtbuffer, "trampolining to 0x%08x\n", address);
		DEBUG_print(txtbuffer, DBG_USBGECKO);
		
		if(!dst_block){
			sprintf(txtbuffer, "block at %08x doesn't exist\n", address&~0xFFF);
			DEBUG_print(txtbuffer, DBG_USBGECKO);
			blocks[address>>12] = malloc(sizeof(PowerPC_block));
			dst_block = blocks[address>>12];
			dst_block->code          = NULL;
			dst_block->code_addr     = NULL;
			dst_block->start_address = address & ~0xFFF;
			dst_block->end_address   = (address & ~0xFFF) + 0x1000;
			init_block(rdram+(((paddr-(address-dst_block->start_address)) & 0x1FFFFFFF)>>2),
				   dst_block);
		}
		
		if(invalid_code_get(address>>12)){
			dst_block->length = 0;
			recompile_block(dst_block);
		}
		
		// Recompute the block offset
		unsigned int (*code)(void);
		code = (unsigned int (*)(void))dst_block->code_addr[(address&0xFFF)>>2];
		
		sprintf(txtbuffer, "Entering dynarec code @ 0x%08x\n", code);
		DEBUG_print(txtbuffer, DBG_USBGECKO);
		
		// TODO: Start counting instructions
		address = code();
		// TODO: Stop counting instructions, add to count
	}
}

void decodeNInterpret(MIPS_instr mips){
	// TODO: Pause counting instructions
	prefetch_opcode(mips);
	interp_ops[MIPS_GET_OPCODE(mips)]();
	// TODO: Resume counting instructions
}
