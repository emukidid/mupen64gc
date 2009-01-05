/* Recompile.c - Recompiles a block of MIPS code to PPC
   by Mike Slegeir for Mupen64-GC
 **********************************************************
   TODO: Try to conform more to the interface mupen64 uses.
         If we have the entire RAM and recompiled code in memory,
           we'll run out of room, we should implement a recompiled
           code cache (e.g. free blocks which haven't been used lately)
         If it's possible, only use the blocks physical addresses (saves space)
 */

#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "../../memory/memory.h"
#include "../interupt.h"
#include "Recompile.h"
#include "Wrappers.h"

extern char invalid_code[0x100000];

static MIPS_instr*    src;
static PowerPC_instr* dst;
static MIPS_instr*    src_last;
static MIPS_instr*    src_first;
static unsigned int*  code_length;
static unsigned int   addr_first;
static unsigned int   addr_last;
static PowerPC_instr* jump_pad;
static jump_node    jump_table[MAX_JUMPS];
static unsigned int current_jump;
static PowerPC_instr** code_addr;
static unsigned char isJmpDst[1024];

static void pass0(PowerPC_block* ppc_block);
static void pass2(PowerPC_block* ppc_block);
//static void genRecompileBlock(PowerPC_block*);
static void genJumpPad(PowerPC_block*);
int resizeCode(PowerPC_block* block, int newSize);

MIPS_instr get_next_src(void) { return *(src++); }
MIPS_instr peek_next_src(void){ return *src;     }
int has_next_src(void){ return (src_last-src) > 0; }
// Hacks for delay slots
 // Undoes a get_next_src
 void unget_last_src(void){ --src; }
 // Used for finding how many instructions were generated
 PowerPC_instr* get_curr_dst(void){ return dst; }
 // Makes sure a branch to a NOP in the delay slot won't crash
 // This should be called ONLY after get_next_src returns a
 //   NOP in a delay slot
 void nop_ignored(void){ code_addr[src-1-src_first] = dst; }
// Returns the MIPS PC
unsigned int get_src_pc(void){ return addr_first + ((src-1-src_first)<<2); }
void set_next_dst(PowerPC_instr i){ *(dst++) = i; ++(*code_length); }

int add_jump(int old_jump, int is_j, int is_out){
	int id = current_jump;
	jump_node* jump = &jump_table[current_jump++];
	jump->old_jump  = old_jump;
	jump->new_jump  = 0;     // This should be filled in when known
	jump->src_instr = src-1; // src points to the next
	jump->dst_instr = dst;   // set_next hasn't happened
	jump->type      = (is_j   ? JUMP_TYPE_J   : 0)
	                | (is_out ? JUMP_TYPE_OUT : 0);
	return id;
}

int add_jump_special(int is_j){
	int id = current_jump;
	jump_node* jump = &jump_table[current_jump++];
	jump->new_jump  = 0;     // This should be filled in when known
	jump->dst_instr = dst;   // set_next hasn't happened
	jump->type      = JUMP_TYPE_SPEC | (is_j ? JUMP_TYPE_J : 0);
	return id;
}

void set_jump_special(int which, int new_jump){
	jump_node* jump = &jump_table[which];
	if(jump->type != JUMP_TYPE_SPEC) return;
	jump->new_jump = new_jump;
}

// Converts a sequence of MIPS instructions to a PowerPC block
void recompile_block(PowerPC_block* ppc_block){
	src_first = ppc_block->mips_code;
	src_last = src_first + (ppc_block->end_address - ppc_block->start_address)/4;
	addr_first = ppc_block->start_address;
	addr_last  = ppc_block->end_address;
	code_addr = ppc_block->code_addr;
	code_length = &ppc_block->length;
	
	pass0(ppc_block);
	
	src = src_first;
	dst = ppc_block->code;
	current_jump = 0;
	start_new_block();
	
	while(has_next_src()){
		// Make sure the code doesn't overflow
		// FIXME: The resize factor may not be optimal
		//          maybe we can make a guess based on
		//          how far along we are now
		if(*code_length + 32 >= ppc_block->max_length)
			resizeCode(ppc_block, ppc_block->max_length * 3/2);
		
		if(isJmpDst[src-src_first]) start_new_mapping();
		
		ppc_block->code_addr[src-src_first] = dst;
		if( convert() == CONVERT_ERROR ){
			printf("Error converting MIPS instruction:\n"
			       "0x%08x   0x%08x\n",
			        ppc_block->start_address + (int)(src-1-src_first)*4, *(src-1));
			//DEBUG_print(txtbuffer, DBG_USBGECKO);
			//int i=16; while(i--) VIDEO_WaitVSync();
		}
	}
	
	// This simplifies jumps and branches out of the block
	genJumpPad(ppc_block);
	
	// Here we recompute jumps and branches
	pass2(ppc_block);
	// Set this block as valid, recompiled code
	invalid_code_set(ppc_block->start_address>>12, 0);
	// Set equivalent blocks as valid, recompiled code pointing to the same code
	if(ppc_block->end_address < 0x80000000 || ppc_block->start_address >= 0xc0000000){	
		unsigned long paddr;
		
		paddr = virtual_to_physical_address(ppc_block->start_address, 2);
		invalid_code_set(paddr>>12, 0);
		blocks[paddr>>12]->code = ppc_block->code;
		blocks[paddr>>12]->code_addr = ppc_block->code_addr;
		blocks[paddr>>12]->length = ppc_block->length;
		blocks[paddr>>12]->max_length = ppc_block->max_length;
		
		paddr += ppc_block->end_address - ppc_block->start_address - 4;
		invalid_code_set(paddr>>12, 0);
		blocks[paddr>>12]->code = ppc_block->code;
		blocks[paddr>>12]->code_addr = ppc_block->code_addr;
		blocks[paddr>>12]->length = ppc_block->length;
		blocks[paddr>>12]->max_length = ppc_block->max_length;
		
	} else {
		unsigned int start = ppc_block->start_address;
		unsigned int end   = ppc_block->end_address;
		if(start >= 0x80000000 && end < 0xa0000000 &&
		   invalid_code_get((start+0x20000000)>>12)){
		   	invalid_code_set((start+0x20000000)>>12, 0);
			blocks[(start+0x20000000)>>12]->code = ppc_block->code;
			blocks[(start+0x20000000)>>12]->code_addr = ppc_block->code_addr;
			blocks[(start+0x20000000)>>12]->length = ppc_block->length;
			blocks[(start+0x20000000)>>12]->max_length = ppc_block->max_length;
		}
		if(start >= 0xa0000000 && end < 0xc0000000 &&
		   invalid_code_get((start-0x20000000)>>12)){
		   	invalid_code_set((start-0x20000000)>>12, 0);
			blocks[(start-0x20000000)>>12]->code = ppc_block->code;
			blocks[(start-0x20000000)>>12]->code_addr = ppc_block->code_addr;
			blocks[(start-0x20000000)>>12]->length = ppc_block->length;
			blocks[(start-0x20000000)>>12]->max_length = ppc_block->max_length;
		}
	}
	// Since this is a fresh block of code,
	// Make sure it wil show up in the ICache
	// FIXME: Flush & Invalidate code	
	//DCFlushRange(ppc_block->code, ppc_block->length*sizeof(PowerPC_instr));
	//ICInvalidateRange(ppc_block->code, ppc_block->length*sizeof(PowerPC_instr));
}

void init_block(MIPS_instr* mips_code, PowerPC_block* ppc_block){
	unsigned int length = (ppc_block->end_address - ppc_block->start_address)/sizeof(MIPS_instr);
	if(!ppc_block->code){
		ppc_block->max_length = 3*length;
		ppc_block->code = malloc(ppc_block->max_length * sizeof(PowerPC_instr));
		mprotect(ppc_block->code, ppc_block->max_length * sizeof(PowerPC_instr), PROT_EXEC|PROT_READ|PROT_WRITE);
	}
	
	if(!ppc_block->code_addr)
		ppc_block->code_addr = malloc(length * sizeof(PowerPC_instr*));
	ppc_block->mips_code = mips_code;
	
	// TODO: We should probably point all equivalent addresses to this block as well
	//         or point to the same code without leaving a dangling pointer
	
	ppc_block->length = 0;
	// We haven't actually recompiled the block
	invalid_code_set(ppc_block->start_address>>12, 1);
	if(ppc_block->end_address < 0x80000000 || ppc_block->start_address >= 0xc0000000){	
		unsigned long paddr;
		
		paddr = virtual_to_physical_address(ppc_block->start_address, 2);
		invalid_code_set(paddr>>12, 1);
		if(!blocks[paddr>>12]){
		     blocks[paddr>>12] = malloc(sizeof(PowerPC_block));
		     blocks[paddr>>12]->code = -1;
		     blocks[paddr>>12]->code_addr = -1;
		     blocks[paddr>>12]->start_address = paddr & ~0xFFF;
		     blocks[paddr>>12]->end_address = (paddr & ~0xFFF) + 0x1000;
		}
		
		paddr += ppc_block->end_address - ppc_block->start_address - 4;
		invalid_code_set(paddr>>12, 1);
		if(!blocks[paddr>>12]){
		     blocks[paddr>>12] = malloc(sizeof(PowerPC_block));
		     blocks[paddr>>12]->code = -1;
		     blocks[paddr>>12]->code_addr = -1;
		     blocks[paddr>>12]->start_address = paddr & ~0xFFF;
		     blocks[paddr>>12]->end_address = (paddr & ~0xFFF) + 0x1000;
		}
		
	} else {
		unsigned int start = ppc_block->start_address;
		unsigned int end   = ppc_block->end_address;
		if(start >= 0x80000000 && end < 0xa0000000 &&
		   invalid_code_get((start+0x20000000)>>12)){
			if(!blocks[(start+0x20000000)>>12]){
				blocks[(start+0x20000000)>>12] = malloc(sizeof(PowerPC_block));
				blocks[(start+0x20000000)>>12]->code = -1;
				blocks[(start+0x20000000)>>12]->code_addr = -1;
				blocks[(start+0x20000000)>>12]->start_address
					= (start+0x20000000) & ~0xFFF;
				blocks[(start+0x20000000)>>12]->end_address
					= ((start+0x20000000) & ~0xFFF) + 0x1000;
			}
		}
		if(start >= 0xa0000000 && end < 0xc0000000 &&
		   invalid_code_get((start-0x20000000)>>12)){
			if (!blocks[(start-0x20000000)>>12]){
				blocks[(start-0x20000000)>>12] = malloc(sizeof(PowerPC_block));
				blocks[(start-0x20000000)>>12]->code = -1;
				blocks[(start-0x20000000)>>12]->code_addr = -1;
				blocks[(start-0x20000000)>>12]->start_address
					= (start-0x20000000) & ~0xFFF;
				blocks[(start-0x20000000)>>12]->end_address
					= ((start-0x20000000) & ~0xFFF) + 0x1000;
			}
		}
	}
}

void deinit_block(PowerPC_block* ppc_block){
	if(ppc_block->code){
		//RecompCache_Free(ppc_block->start_address>>12);
		free(ppc_block->code);
		ppc_block->code = NULL;
	}
	if(ppc_block->code_addr){
		free(ppc_block->code_addr);
		ppc_block->code_addr = NULL;
	}
	invalid_code_set(ppc_block->start_address>>12, 1);
	
	// TODO: We need to mark all equivalent addresses as invalid
}

int is_j_out(int branch, int is_aa){
	if(is_aa)
		return ((branch << 2 | (addr_first & 0xF0000000)) < addr_first ||
		        (branch << 2 | (addr_first & 0xF0000000)) > addr_last);
	else {
		int dst_instr = (src - src_first) + branch;
		return (dst_instr < 0 || dst_instr >= (addr_last-addr_first)>>2);
	}
}

// Pass 2 fills in all the new addresses
static void pass2(PowerPC_block* ppc_block){
	int i;
	PowerPC_instr* current;
	for(i=0; i<current_jump; ++i){
		current = jump_table[i].dst_instr;
		
		// Special jump, its been filled out
		if(jump_table[i].type & JUMP_TYPE_SPEC){
			if(!(jump_table[i].type & JUMP_TYPE_J)){
				// We're filling in a branch instruction
				*current &= ~(PPC_BD_MASK << PPC_BD_SHIFT);
				PPC_SET_BD(*current, jump_table[i].new_jump);
			} else {
				// We're filling in a jump instrucion
				*current &= ~(PPC_LI_MASK << PPC_LI_SHIFT);
				PPC_SET_LI(*current, jump_table[i].new_jump);
			}
			continue;
		}
		
		if(jump_table[i].type & JUMP_TYPE_OUT){
			unsigned int jump_address = (jump_table[i].old_jump << 2) |
			                            ((unsigned int)ppc_block->start_address & 0xF0000000);
			// We're jumping out of this block
			/*sprintf(txtbuffer,"Trampolining out to 0x%08x\n", jump_address);
			DEBUG_print(txtbuffer, DBG_USBGECKO);*/
			
			// b <jump_pad>
			int distance = ((unsigned int)jump_pad-(unsigned int)current)>>2;
			if(jump_table[i].type & JUMP_TYPE_J){
				*current &= ~(PPC_LI_MASK << PPC_LI_SHIFT);
				PPC_SET_LI(*current, distance);
			} else {
				*current &= ~(PPC_BD_MASK << PPC_BD_SHIFT);
				PPC_SET_BD(*current, distance);
			}
			
		} else if(!(jump_table[i].type & JUMP_TYPE_J)){ // Branch instruction
			int jump_offset = (unsigned int)jump_table[i].old_jump + 
				         ((unsigned int)jump_table[i].src_instr - (unsigned int)src_first)/4;
			
			jump_table[i].new_jump = ppc_block->code_addr[jump_offset] - current;

			/*sprintf(txtbuffer,"Converting old_jump = %d, to new_jump = %d\n",
			       jump_table[i].old_jump, jump_table[i].new_jump);
			DEBUG_print(txtbuffer, DBG_USBGECKO);*/
			
			*current &= ~(PPC_BD_MASK << PPC_BD_SHIFT);
			PPC_SET_BD(*current, jump_table[i].new_jump);
			
		} else { // Jump instruction
			// The jump_address is actually calculated with the delay slot address
			unsigned int jump_address = (jump_table[i].old_jump << 2) |
			                            ((unsigned int)ppc_block->start_address & 0xF0000000);
			
			// We're jumping within this block, find out where
			int jump_offset = (jump_address - ppc_block->start_address) >> 2;
			jump_table[i].new_jump = ppc_block->code_addr[jump_offset] - current;

			/*sprintf(txtbuffer,"Jumping to 0x%08x; jump_offset = %d\n", jump_address, jump_offset);
			DEBUG_print(txtbuffer, DBG_USBGECKO);*/
			*current &= ~(PPC_LI_MASK << PPC_LI_SHIFT);
			PPC_SET_LI(*current, jump_table[i].new_jump);
			
		}
	}
}

static void pass0(PowerPC_block* ppc_block){
	unsigned int pc = addr_first >> 2;
	int i;
	// Zero out the jump destinations table
	for(i=0; i<1024; ++i) isJmpDst[i] = 0;
	// Go through each instruction and map every branch instruction's destination
	for(src = src_first; src < src_last; ++src, ++pc){
		int opcode = MIPS_GET_OPCODE(*src);
		if(opcode == MIPS_OPCODE_J || opcode == MIPS_OPCODE_JAL){
			unsigned int li = MIPS_GET_LI(*src);
			++src; ++pc;
			if(!is_j_out(li, 1)){
				assert( ((li&0x3FF) >= 0) && ((li&0x3FF) < 1024) );
				isJmpDst[ li & 0x3FF ] = 1;
			}
		} else if(opcode == MIPS_OPCODE_BEQ   ||
                  opcode == MIPS_OPCODE_BNE   ||
                  opcode == MIPS_OPCODE_BLEZ  ||
                  opcode == MIPS_OPCODE_BGTZ  ||
                  opcode == MIPS_OPCODE_BEQL  ||
                  opcode == MIPS_OPCODE_BNEL  ||
                  opcode == MIPS_OPCODE_BLEZL ||
                  opcode == MIPS_OPCODE_BGTZL ||
                  opcode == MIPS_OPCODE_B     ){
        	int bd = MIPS_GET_IMMED(*src);
        	++src; ++pc;
        	bd |= (bd & 0x8000) ? 0xFFFF0000 : 0; // sign extend
        	if(!is_j_out(bd, 0)){
        		assert( ((pc & 0x3FF) + bd >= 0) && ((pc & 0x3FF) + bd < 1024) );
        		isJmpDst[ (pc & 0x3FF) + bd ] = 1;
        	}
		}
	}
}

extern int stop;
inline unsigned long update_invalid_addr(unsigned long addr);
void jump_to(unsigned int address){
#if 0
	if(stop) return;
	PowerPC_block* dst_block = blocks[address>>12];
	unsigned long paddr = update_invalid_addr(address);
	if(!paddr) return;
	
	// Check for interrupts
	update_count();
	if(next_interupt <= Count) gen_interupt();
	
	sprintf(txtbuffer, "jump_to 0x%08x\n", address);
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
	// TODO: If we deinit blocks we haven't used recently, we should do something like:
	//         new_block(dst_block); // This checks if its not inited, if not, it inits
	
	if(invalid_code_get(address>>12)){
		dst_block->length = 0;
		recompile_block(dst_block);
	}
	
	// Support the cache
	//update_lru();
	//lru[address>>12] = 0;
	
	// Recompute the block offset
	int offset = 0;
	if((address&0xFFF) > 0)
		offset = dst_block->code_addr[(address&0xFFF)>>2] - dst_block->code;
	
	sprintf(txtbuffer, "Entering dynarec code @ 0x%08x\n",
	         dst_block->code_addr[(address&0xFFF)>>2]);
	DEBUG_print(txtbuffer, DBG_USBGECKO);
	/*int i;
	for(i=0; i<6; ++i) VIDEO_WaitVSync(); // Wait for output to propagate*/
	
	start(dst_block, offset);
#else
	printf("jump_to(%08x) called! exiting.\n", address);
	//DEBUG_print(txtbuffer, DBG_USBGECKO);
	stop = 1;
#endif
}
extern unsigned long jump_to_address;
void dyna_jump(){ jump_to(jump_to_address); }
void dyna_stop(){ }
void jump_to_func(){ jump_to(jump_to_address); }

// Warning: This is a slow operation, try not to use it
int resizeCode(PowerPC_block* block, int newSize){
	if(!block->code) return 0;
	
	// Creating the new block and copying the code
	PowerPC_instr* oldCode = block->code;
	block->code = realloc(block->code, newSize * sizeof(PowerPC_instr));
	mprotect(block->code, block->max_length * sizeof(PowerPC_instr), PROT_EXEC|PROT_READ|PROT_WRITE);
	if(!block->code) return 0;
	
	// Readjusting pointers
	// Optimization: Sepp256 suggested storing offsets instead of pointers
	//                 then this step won't be necessary
	dst = block->code + (dst - oldCode);
	int i;
	for(i=0; i<(block->end_address - block->start_address)/4; ++i)
		block->code_addr[i] = block->code + (block->code_addr[i] - oldCode);
	for(i=0; i<current_jump; ++i)
		jump_table[i].dst_instr = block->code + (jump_table[i].dst_instr - oldCode);
	
	block->max_length = newSize;
	return newSize;
}

extern unsigned long instructionCount;
static void genJumpPad(PowerPC_block* ppc_block){
	PowerPC_instr ppc = NEW_PPC_INSTR();
	
	// XXX: Careful there won't be a block overflow
	if(*code_length + 16 >= ppc_block->max_length)
			resizeCode(ppc_block, ppc_block->max_length + 16);
	// Set the next address to the first address in the next block if
	//   we've really reached the end of the block, not jumped to the pad
	GEN_LIS(ppc, 3, ppc_block->end_address>>16);
	set_next_dst(ppc);
	GEN_LI(ppc, 3, 3, ppc_block->end_address);
	set_next_dst(ppc);
	
	jump_pad = dst;
	
	// Restore any saved registers
	// Restore cr
	GEN_LWZ(ppc, DYNAREG_REG, DYNAOFF_CR, 1);
	set_next_dst(ppc);
	GEN_MTCR(ppc, DYNAREG_REG);
	set_next_dst(ppc);
	// Restore r14
	GEN_LWZ(ppc, DYNAREG_REG, DYNAOFF_REG, 1);
	set_next_dst(ppc);
	// Restore r15
	GEN_LWZ(ppc, DYNAREG_ZERO, DYNAOFF_ZERO, 1);
	set_next_dst(ppc);
	// Restore r16
	GEN_LWZ(ppc, DYNAREG_INTERP, DYNAOFF_INTERP, 1);
	set_next_dst(ppc);
	// Actually set the instruction count
	GEN_LIS(ppc, 4, (unsigned int)&instructionCount>>16);
	set_next_dst(ppc);
	GEN_ORI(ppc, 4, 4, (unsigned int)&instructionCount);
	set_next_dst(ppc);
	// Store the value to instructionCount
	GEN_STW(ppc, DYNAREG_ICOUNT, 0, 4);
	set_next_dst(ppc);
	// Restore r17
	GEN_LWZ(ppc, DYNAREG_ICOUNT, DYNAOFF_ICOUNT, 1);
	set_next_dst(ppc);
	// Restore the sp
	GEN_LWZ(ppc, 1, 0, 1);
	set_next_dst(ppc);
	// return destination
	GEN_BLR(ppc,0);
	set_next_dst(ppc);
}

