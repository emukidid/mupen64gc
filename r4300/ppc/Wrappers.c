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
extern unsigned long instructionCount;
extern void (*interp_ops[64])(void);
inline unsigned long update_invalid_addr(unsigned long addr);


/* Recompiled code stack frame:
 *  $sp+28  | old r18 (new r18 holds &last_addr)
 *  $sp+24  | old r17 (new r17 holds dyna_update_count)
 *  $sp+20  | old r16 (new r16 holds decodeNInterpret)
 *  $sp+16  | old r15 (new r15 holds 0)
 *  $sp+12  | old r14 (new r14 holds reg)
 *  $sp+8   | old cr
 *  $sp+4   | old lr
 *  $sp	    | old sp
 */

#define DYNAREC_PRELUDE() \
	__asm__ __volatile__ ( \
		"stwu	1, -32(1) \n" \
		"stw	14, 12(1) \n" \
		"mfcr	14        \n" \
		"stw	14, 8(1)  \n" \
		"mr	14, %0    \n" \
		"stw	15, 16(1) \n" \
		"addi	15, 0, 0  \n" \
		"stw	16, 20(1) \n" \
		"mr	16, %1    \n" \
		"stw	17, 24(1) \n" \
		"mr	17, %2    \n" \
		"stw	18, 28(1) \n" \
		"mr	18, %3    \n" \
		:: "r" (reg), "r" (decodeNInterpret), \
		   "r" (dyna_update_count), "r" (&last_addr) )


void dynarec(unsigned int address){
	while(!stop){
		PowerPC_block* dst_block = blocks[address>>12];
		unsigned long paddr = update_invalid_addr(address);
		
		sprintf(txtbuffer, "trampolining to 0x%08x\n", address);
		DEBUG_print(txtbuffer, DBG_USBGECKO);
		
		if(!paddr){ stop=1; return; }
		
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
			sprintf(txtbuffer, "block at %08x is invalid\n", address&~0xFFF);
			DEBUG_print(txtbuffer, DBG_USBGECKO);
			dst_block->length = 0;
			recompile_block(dst_block);
		}
		
		// Recompute the block offset
		unsigned int (*code)(void);
		code = (unsigned int (*)(void))dst_block->code_addr[(address&0xFFF)>>2];
		
		sprintf(txtbuffer, "Entering dynarec code @ 0x%08x\n", code);
		DEBUG_print(txtbuffer, DBG_USBGECKO);
		
		// FIXME: Try actually saving the lr now: use &&label after code() and then mr %address, 3
		DYNAREC_PRELUDE();
		address = code();
		
		last_addr = interp_addr = address;
		// Check for interrupts
		if(next_interupt <= Count){
			gen_interupt();
			address = interp_addr;
		}
	}
}

unsigned int decodeNInterpret(MIPS_instr mips, unsigned int pc,
                              int isDelaySlot){
	delay_slot = isDelaySlot; // Make sure we set delay_slot properly
	PC->addr = interp_addr = pc;
	prefetch_opcode(mips);
	interp_ops[MIPS_GET_OPCODE(mips)]();
	
	return interp_addr != pc + 4 ? interp_addr : 0;
}

int dyna_update_count(unsigned int pc){
	Count += (pc - last_addr)/2;
	last_addr = pc;
	
	return next_interupt - Count;
}

