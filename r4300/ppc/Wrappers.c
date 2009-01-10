/* Wrappers.c - Dynarec trampoline and other functions to simplify access from dynarec
   by Mike Slegeir for Mupen64-GC
 */

#include <stdlib.h>
#include "../../memory/memory.h"
#include "../interupt.h"
#include "../r4300.h"
#include "Wrappers.h"

extern int stop;
extern unsigned long instructionCount;
extern void (*interp_ops[64])(void);
inline unsigned long update_invalid_addr(unsigned long addr);


/* Recompiled code stack frame:
 *  $sp+28  |
 *  $sp+24  | old r17 (new r17 holds instruction count)
 *  $sp+20  | old r16 (new r16 holds decodeNInterpret)
 *  $sp+16  | old r15 (new r15 holds 0)
 * 	$sp+12	| old r14 (new r14 holds reg)
 * 	$sp+8	| old cr
 * 	$sp+4	| old lr
 * 	$sp		| old sp
 */

#define DYNAREC_PRELUDE() \
	__asm__ __volatile__ ( \
		"stwu	1, -32(1) \n" \
		"stw	14, 12(1) \n" \
		"mfcr	14        \n" \
		"stw	14, 8(1)  \n" \
		"mr		14, %0    \n" \
		"stw	15, 16(1) \n" \
		"addi	15, 0, 0  \n" \
		"stw	16, 20(1) \n" \
		"mr		16, %1    \n" \
		"stw	17, 24(1) \n" \
		"addi	17, 0, 0  \n" \
		:: "r" (reg), "r" (decodeNInterpret) )


void dynarec(unsigned int address){
	while(!stop){
		PowerPC_block* dst_block = blocks[address>>12];
		unsigned long paddr = update_invalid_addr(address);
		if(!paddr) return;
		
		// Check for interrupts
		update_count();
		if(next_interupt <= Count) gen_interupt();
		
		printf("trampolining to 0x%08x\n", address);
		//DEBUG_print(txtbuffer, DBG_USBGECKO);
		
		if(!dst_block){
			printf("block at %08x doesn't exist\n", address&~0xFFF);
			//DEBUG_print(txtbuffer, DBG_USBGECKO);
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
			printf("block at %08x is invalid\n", address&~0xFFF);
			//DEBUG_print(txtbuffer, DBG_USBGECKO);
			dst_block->length = 0;
			recompile_block(dst_block);
		}
		
		// Recompute the block offset
		unsigned int (*code)(void);
		code = (unsigned int (*)(void))dst_block->code_addr[(address&0xFFF)>>2];
		
		printf("Entering dynarec code @ 0x%08x\n", code);
		//DEBUG_print(txtbuffer, DBG_USBGECKO);
		
		// FIXME: Try actually saving the lr now: use &&label after code() and then mr %address, 3
		DYNAREC_PRELUDE();
		address = code();
	}
}

unsigned int decodeNInterpret(MIPS_instr mips, unsigned int pc,
                              unsigned int count, int isDelaySlot){
	instructionCount = count; // Update the value for the instruction count
	delay_slot = isDelaySlot; // Make sure we set delay_slot properly
	PC->addr = interp_addr = pc;
	prefetch_opcode(mips);
	interp_ops[MIPS_GET_OPCODE(mips)]();
	
	// Check for interrupts
	update_count();
	if(next_interupt <= Count) gen_interupt();
	
	return interp_addr != pc + 4 ? interp_addr : 0;
}

