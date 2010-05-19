/* Wrappers.c - Dynarec trampoline and other functions to simplify access from dynarec
   by Mike Slegeir for Mupen64-GC
 */

#include <stdlib.h>
#include "../../memory/memory.h"
#include "../interupt.h"
#include "../r4300.h"
#include "../Recomp-Cache.h"
#include "Recompile.h"
#include "Wrappers.h"

#include <assert.h>

extern int stop;
extern unsigned long instructionCount;
extern void (*interp_ops[64])(void);
inline unsigned long update_invalid_addr(unsigned long addr);
unsigned int dyna_check_cop1_unusable(unsigned int, int);
unsigned int dyna_mem(unsigned int, unsigned int, memType, unsigned int, int);

int noCheckInterrupt = 0;

static PowerPC_instr* link_branch = NULL;
static PowerPC_func* last_func;

/* Recompiled code stack frame:
 *  $sp+12  |
 *  $sp+8   | old cr
 *  $sp+4   | old lr
 *  $sp	    | old sp
 */

inline unsigned int dyna_run(PowerPC_func* func, unsigned int (*code)(void)){
	unsigned int naddr;
	PowerPC_instr* return_addr;

	__asm__ volatile(
		// Create the stack frame for code
		"stwu	1, -16(1) \n"
		"mfcr	14        \n"
		"stw	14, 8(1)  \n"
		// Setup saved registers for code
		"mr	14, %0    \n"
		"mr	15, %1    \n"
		"mr	16, %2    \n"
		"mr	17, %3    \n"
		"mr	18, %4    \n"
		"mr	19, %5    \n"
		"mr	20, %6    \n"
		"mr	21, %7    \n"
		"mr	22, %8    \n"
		"addi	23, 0, 0  \n"
		:: "r" (reg), "r" (reg_cop0),
		   "r" (reg_cop1_simple), "r" (reg_cop1_double),
		   "r" (&FCR31), "r" (rdram),
		   "r" (&last_addr), "r" (&next_interupt),
		   "r" (func)
		: "14", "15", "16", "17", "18", "19", "20", "21", "22", "23");

	// naddr = code();
	__asm__ volatile(
		// Save the lr so the recompiled code won't have to
		"bl	4         \n"
		"mtctr	%4        \n"
		"mflr	4         \n"
		"addi	4, 4, 20  \n"
		"stw	4, 20(1)  \n"
		// Execute the code
		"bctrl           \n"
		"mr	%0, 3     \n"
		// Get return_addr, link_branch, and last_func
		"lwz	%2, 20(1) \n"
		"mflr	%1        \n"
		"mr	%3, 22    \n"
		// Pop the stack
		"lwz	1, 0(1)   \n"
		: "=r" (naddr), "=r" (link_branch), "=r" (return_addr),
		  "=r" (last_func)
		: "r" (code)
		: "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "22");

	link_branch = link_branch == return_addr ? NULL : link_branch - 1;
	
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
			else if(paddr >= 0xa4000000) base = SP_DMEM;
			else base = rdram;
			init_block(base+offset, dst_block);

		} else if(invalid_code_get(address>>12)){
			invalidate_block(dst_block);
		}

		PowerPC_func* func = find_func(&dst_block->funcs, address&0xFFFF);

		if(!func || !func->code_addr[((address&0xFFFF)-func->start_addr)>>2]){
			printf("code at %08x is not compiled\n", address);
			start_section(COMPILER_SECTION);
			func = recompile_block(dst_block, address);
			end_section(COMPILER_SECTION);
		} else {
#ifdef USE_RECOMP_CACHE
			RecompCache_Update(func);
#endif
		}

		int index = ((address&0xFFFF) - func->start_addr)>>2;

		// Recompute the block offset
		unsigned int (*code)(void);
		code = (unsigned int (*)(void))func->code_addr[index];
		
		// Create a link if possible
		if(link_branch && !func_was_freed(last_func))
			RecompCache_Link(last_func, link_branch, func, code);
		clear_freed_funcs();
		
		address = dyna_run(func, code);

		if(!noCheckInterrupt || skip_jump){
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
}

static void invalidate_func(unsigned int addr){
	PowerPC_block* block = blocks[addr>>12];
	PowerPC_func* func = find_func(&block->funcs, addr&0xffff);
	if(func)
		RecompCache_Free(block->start_address | func->start_addr);
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
		case MEM_LDC1:
			read_dword_in_memory();
			*((long long*)reg_cop1_double[value]) = dyna_rdword;
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
		case MEM_SD:
			dword = reg[value];
			write_dword_in_memory();
			check_memory();
			break;
		case MEM_SWC1:
			word = *((long*)reg_cop1_simple[value]);
			write_word_in_memory();
			check_memory();
			break;
		case MEM_SDC1:
			dword = *((unsigned long long*)reg_cop1_double[value]);
			write_dword_in_memory();
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

