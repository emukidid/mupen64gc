/* Wrappers.c - Dynarec trampoline and other functions to simplify access from dynarec
   by Mike Slegeir for Mupen64-GC
 */

#include <stdlib.h>
#include "../../memory/memory.h"
#include "../interupt.h"
#include "../r4300.h"
#include "Wrappers.h"

#include <assert.h>

extern int stop;
extern unsigned long instructionCount;
extern void (*interp_ops[64])(void);
inline unsigned long update_invalid_addr(unsigned long addr);
unsigned int dyna_check_cop1_unusable(unsigned int, int);
unsigned int dyna_mem(unsigned int, unsigned int, memType, unsigned int, int);

int noCheckInterrupt = 0;

/* Recompiled code stack frame:
 *  $sp+12  |
 *  $sp+8   | old cr
 *  $sp+4   | old lr
 *  $sp	    | old sp
 */

inline unsigned int dyna_run(unsigned int (*code)(void)){
	__asm__ volatile(
		"stwu	1, -16(1) \n"
		"mfcr	14        \n"
		"stw	14, 8(1)  \n"
		"mr	14, %0    \n"
		"addi	15, 0, 0  \n"
		"mr	16, %1    \n"
		"mr	17, %2    \n"
		"mr	18, %3    \n"
		"mr	19, %4    \n"
		"mr	20, %5    \n"
		"mr	21, %6    \n"
		"mr	22, %7    \n"
		"mr	23, %8    \n"
		"mr	24, %9    \n"
		:: "r" (reg), "r" (decodeNInterpret),
		   "r" (dyna_update_count), "r" (&last_addr),
		   "r" (rdram), "r" (dyna_mem),
		   "r" (reg_cop1_simple), "r" (reg_cop1_double),
		   "r" (&FCR31), "r" (dyna_check_cop1_unusable)
		: "14", "15", "16", "17", "18", "19", "20", "21",
		  "22", "23", "24");

	unsigned int naddr = code();

	__asm__ volatile("lwz	1, 0(1)\n");

	return naddr;
}

void dynarec(unsigned int address){
	while(!stop){
		PowerPC_block* dst_block = blocks[address>>12];

		//printf("trampolining to 0x%08x from ~%08x\n", address, interp_addr);

		unsigned long paddr = update_invalid_addr(address);
		if(!paddr){
			printf("Caught you at last fucker!\nStay away from %08x\n", address);
			return;
		}

		if(!dst_block){
			printf("block at %08x doesn't exist\n", address&~0xFFF);
			blocks[address>>12] = malloc(sizeof(PowerPC_block));
			dst_block = blocks[address>>12];
			//dst_block->code_addr     = NULL;
			dst_block->funcs         = NULL;
			dst_block->start_address = address & ~0xFFF;
			dst_block->end_address   = (address & ~0xFFF) + 0x1000;

			unsigned int offset = ((paddr-(address-dst_block->start_address))&0x0FFFFFFF)>>2;
			unsigned int* base;
			if(paddr > 0xb0000000) base = rom;
			else base = rdram;
			init_block(base+offset, dst_block);

		} else if(invalid_code_get(address>>12)){
			invalidate_block(dst_block);
		}

		PowerPC_func_node* fn;
		for(fn = dst_block->funcs; fn != NULL; fn = fn->next)
			if((address&0xFFFF) >= fn->function->start_addr &&
			   ((address&0xFFFF) < fn->function->end_addr ||
			    fn->function->end_addr == 0)) break;
		if(!fn || !fn->function->code_addr[((address&0xFFFF) -
		                                    fn->function->start_addr)>>2]){
			printf("code at %08x is not compiled\n", address);
			start_section(COMPILER_SECTION);
			recompile_block(dst_block, address);
			end_section(COMPILER_SECTION);
		} else {
#ifdef USE_RECOMP_CACHE
			RecompCache_Update(address);
#endif
		}

		for(fn = dst_block->funcs; fn != NULL; fn = fn->next)
			if((address&0xFFFF) >= fn->function->start_addr &&
			   ((address&0xFFFF) < fn->function->end_addr ||
			    fn->function->end_addr == 0)) break;
		assert(fn);
		int index = ((address&0xFFFF) - fn->function->start_addr)>>2;

		// Recompute the block offset
		unsigned int (*code)(void);
		code = (unsigned int (*)(void))fn->function->code_addr[index];
		address = dyna_run(code);

		if(!noCheckInterrupt){
			last_addr = interp_addr = address;
			// Check for interrupts
			if(next_interupt <= Count){
				gen_interupt();
				//printf("gen_interupt from trampoline from %08x to %08x\n", address, interp_addr);
				address = interp_addr;
			}
		}
		noCheckInterrupt = 0;
	}
	interp_addr = address;
}

unsigned int decodeNInterpret(MIPS_instr mips, unsigned int pc,
                              int isDelaySlot){
	delay_slot = isDelaySlot; // Make sure we set delay_slot properly
	PC->addr = interp_addr = pc;
	prefetch_opcode(mips);
	interp_ops[MIPS_GET_OPCODE(mips)]();
	delay_slot = 0;

	if(interp_addr != pc + 4) noCheckInterrupt = 1;

	return interp_addr != pc + 4 ? interp_addr : 0;
}

#ifdef COMPARE_CORE
int dyna_update_count(unsigned int pc, int isDelaySlot){
#else
int dyna_update_count(unsigned int pc){
#endif
	/*if(Count >= 0xf98000)
		printf("Tracing at %08x (last_addr: %08x, Count: %x)\n", pc, last_addr, Count);*/
	if(pc < last_addr)
		printf("pc (%08x) < last_addr (%08x)\n", pc, last_addr);

	Count += (pc - last_addr)/2;
	last_addr = pc;

#ifdef COMPARE_CORE
	if(isDelaySlot){
		interp_addr = pc;
		//if(Count > 0x1790000) printf("compare_core @ %08x (dyna)\n", pc);
		compare_core();
	}
#endif

	return next_interupt - Count;
}

unsigned int dyna_check_cop1_unusable(unsigned int pc, int isDelaySlot){
	// Check if FP unusable bit is set
	if(!(Status & 0x20000000)){
		// Set state so it can be recovered after exception
		delay_slot = isDelaySlot;
		PC->addr = interp_addr = pc;
		// Take a FP unavailable exception
		Cause = (11 << 2) | 0x10000000;
		exception_general();
		// Reset state
		delay_slot = 0;
		noCheckInterrupt = 1;
		// Return the address to trampoline to
		return interp_addr;
	} else
		return 0;
}

static void invalidate_func(unsigned int addr){
	PowerPC_block* block = blocks[address>>12];
	PowerPC_func_node* fn;
	for(fn = block->funcs; fn != NULL; fn = fn->next){
		if((addr&0xffff) >= fn->function->start_addr &&
		   (addr&0xffff) <  fn->function->end_addr){
			RecompCache_Free(block->start_address |
			                 fn->function->start_addr);
			break;
		}
	}
}

#define check_memory() \
	if(!invalid_code_get(address>>12)/* && \
	   blocks[address>>12]->code_addr[(address&0xfff)>>2]*/) \
		invalidate_func(address);

unsigned int dyna_mem(unsigned int value, unsigned int addr,
                      memType type, unsigned int pc, int isDelaySlot){
	static unsigned long long dyna_rdword;

	address = addr;
	rdword = &dyna_rdword;
	PC->addr = interp_addr = pc;
	delay_slot = isDelaySlot;

	switch(type){
		case MEM_LW:
			read_word_in_memory();
			reg[value] = (long long)((long)dyna_rdword);
			break;
		case MEM_LWU:
			read_word_in_memory();
			reg[value] = (unsigned long long)((long)dyna_rdword);
			break;
		case MEM_LH:
			read_hword_in_memory();
			reg[value] = (long long)((short)dyna_rdword);
			break;
		case MEM_LHU:
			read_hword_in_memory();
			reg[value] = (unsigned long long)((unsigned short)dyna_rdword);
			break;
		case MEM_LB:
			read_byte_in_memory();
			reg[value] = (long long)((signed char)dyna_rdword);
			break;
		case MEM_LBU:
			read_byte_in_memory();
			reg[value] = (unsigned long long)((unsigned char)dyna_rdword);
			break;
		case MEM_LD:
			read_dword_in_memory();
			reg[value] = dyna_rdword;
			break;
		case MEM_LWC1:
			read_word_in_memory();
			*((long*)reg_cop1_simple[value]) = (long)dyna_rdword;
			break;
		case MEM_SW:
			word = value;
			write_word_in_memory();
			check_memory();
			break;
		case MEM_SH:
			hword = value;
			write_hword_in_memory();
			check_memory();
			break;
		case MEM_SB:
			byte = value;
			write_byte_in_memory();
			check_memory();
			break;
		default:
			stop = 1;
			break;
	}
	delay_slot = 0;

	if(interp_addr != pc) noCheckInterrupt = 1;

	return interp_addr != pc ? interp_addr : 0;
}

