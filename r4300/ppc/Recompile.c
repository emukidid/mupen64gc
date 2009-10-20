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
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "../../gc_memory/memory.h"
#include "../Invalid_Code.h"
#include "../interupt.h"
#include "Recompile.h"
#include "../Recomp-Cache.h"
#include "Wrappers.h"

#include "../../gui/DEBUG.h"

static MIPS_instr*    src;
static PowerPC_instr* dst;
static MIPS_instr*    src_last;
static MIPS_instr*    src_first;
static unsigned int   code_length;
static unsigned int   addr_first;
static unsigned int   addr_last;
static jump_node    jump_table[MAX_JUMPS];
static unsigned int current_jump;
static PowerPC_instr** code_addr;
static unsigned char isJmpDst[1024];

static int pass0(PowerPC_block* ppc_block);
static void pass2(PowerPC_block* ppc_block);
//static void genRecompileBlock(PowerPC_block*);
static void genJumpPad(void);
int resizeCode(PowerPC_block* block, PowerPC_func* func, int newSize);
void invalidate_block(PowerPC_block* ppc_block);

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
 void nop_ignored(void){ if(src<src_last) code_addr[src-1-src_first] = dst; }
 // Returns whether the current src instruction is branched to
 int is_j_dst(void){ return isJmpDst[(get_src_pc()&0xfff)>>2]; }
// Returns the MIPS PC
unsigned int get_src_pc(void){ return addr_first + ((src-1-src_first)<<2); }
void set_next_dst(PowerPC_instr i){ *(dst++) = i; ++code_length; }
// Adjusts the code_addr for the current instruction to account for flushes
void reset_code_addr(void){ if(src<=src_last) code_addr[src-1-src_first] = dst; } // FIXME: < or <=??

int add_jump(int old_jump, int is_j, int is_call){
	int id = current_jump;
	jump_node* jump = &jump_table[current_jump++];
	jump->old_jump  = old_jump;
	jump->new_jump  = 0;     // This should be filled in when known
	jump->src_instr = src-1; // src points to the next
	jump->dst_instr = dst;   // set_next hasn't happened
	jump->type      = (is_j    ? JUMP_TYPE_J    : 0)
	                | (is_call ? JUMP_TYPE_CALL : 0);
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
	if(!(jump->type & JUMP_TYPE_SPEC)) return;
	jump->new_jump = new_jump;
}

// Converts a sequence of MIPS instructions to a PowerPC block
PowerPC_func* recompile_block(PowerPC_block* ppc_block, unsigned int addr){
	src_first = ppc_block->mips_code + ((addr&0xfff)>>2);
	addr_first = ppc_block->start_address + (addr&0xfff);
	code_addr = NULL; // Just to make sure this isn't used here

	int need_pad = pass0(ppc_block); // Sets src_last, addr_last

	code_length = 0;
	unsigned int max_length = addr_last - addr_first; // 4x size

	// Create a PowerPC_func for this function
	PowerPC_func* func = malloc(sizeof(PowerPC_func));
	func->start_addr = addr_first&0xffff;
	func->end_addr = addr_last&0xffff;
	func->code = NULL;
	func->holes = NULL;
	func->code_addr = NULL;
	// Create a corresponding PowerPC_func_node to add to ppc_block->funcs
#if 0
	PowerPC_func_node* node = malloc(sizeof(PowerPC_func_node));
	node->function = func;
	node->next = ppc_block->funcs;
	ppc_block->funcs = node;
#endif
	int needInsert = 1;

	// Check for and remove any overlapping functions
	PowerPC_func* handle_overlap(PowerPC_func_node** node, PowerPC_func* func){
		if(!(*node)) return func;
		// Check for any potentially overlapping functions to the left or right
		if((*node)->function->end_addr >= func->start_addr ||
		   !(*node)->function->end_addr)
			func = handle_overlap(&(*node)->left, func);
		if((*node)->function->start_addr < func->end_addr ||
		   !func->end_addr)
			func = handle_overlap(&(*node)->right, func);
		// Check for overlap with this function
		if((*node)->function->start_addr > func->start_addr &&
		   (*node)->function->end_addr == func->end_addr){
			// (*node)->function is a hole in func
			PowerPC_func_hole_node* hole = malloc(sizeof(PowerPC_func_hole_node));
			hole->addr = (*node)->function->start_addr;
			hole->next = func->holes;
			func->holes = hole;
			// Add all holes from the hole
			// Get to the end of this func->holes
			PowerPC_func_hole_node* fhn;
			for(fhn=func->holes; fhn->next; fhn=fhn->next);
			// Add fn->function's holes to the end func->holes
			fhn->next = (*node)->function->holes;
			// Make sure those holes aren't freed
			(*node)->function->holes = NULL;
			// Free the hole
			RecompCache_Free(ppc_block->start_address |
			                 (*node)->function->start_addr);

		} else if(func->start_addr > (*node)->function->start_addr &&
				  func->end_addr == (*node)->function->end_addr){
			// func is a hole in fn->function
			PowerPC_func_hole_node* hole = malloc(sizeof(PowerPC_func_hole_node));
			hole->addr = func->start_addr;
			hole->next = (*node)->function->holes;
			(*node)->function->holes = hole;
			// Free up func and its node
#if 0
			ppc_block->funcs = node->next;
			free(node);
#else
			needInsert = 0;
#endif
			free(func);
			// Move all our pointers to the outer function
			func = (*node)->function;
			max_length = RecompCache_Size(func) / 4;
			addr = ppc_block->start_address + (func->start_addr&0xfff);
			src_first = ppc_block->mips_code + ((addr&0xfff)>>2);
			addr_first = ppc_block->start_address + (addr&0xfff);
			need_pad = pass0(ppc_block);
			RecompCache_Update(func);
			// There cannot be another overlapping function
			//break;

		} else if((!(*node)->function->end_addr || // Runs to end
				   func->start_addr < (*node)->function->end_addr) &&
				  (!func->end_addr || // Function runs to the end of a 0xfxxx
				   func->end_addr   > (*node)->function->start_addr))
			// We have some other non-containment overlap
			RecompCache_Free(ppc_block->start_address |
							 (*node)->function->start_addr);
		return func;
	}
	func = handle_overlap(&ppc_block->funcs, func);
	if(needInsert) insert_func(&ppc_block->funcs, func);

	if(!func->code){
		// We aren't simply recompiling from a hole
#ifdef USE_RECOMP_CACHE
		RecompCache_Alloc(max_length * sizeof(PowerPC_instr), addr, func);
#else
		func->code = malloc(max_length * sizeof(PowerPC_instr));
#endif
	}

	PowerPC_func_hole_node* hole;
	for(hole = func->holes; hole != NULL; hole = hole->next){
		isJmpDst[ (hole->addr&0xfff) >> 2 ] = 1;
	}

	src = src_first;
	dst = func->code;
	current_jump = 0;
	code_addr = func->code_addr;
	start_new_block();
	isJmpDst[src-ppc_block->mips_code] = 1;

	while(has_next_src()){
		unsigned int offset = src - ppc_block->mips_code;
		// Make sure the code doesn't overflow
		// FIXME: The resize factor may not be optimal
		//          maybe we can make a guess based on
		//          how far along we are now
		if(code_length + 64 >= max_length){
			max_length = max_length * 3/2 + 64;
			resizeCode(ppc_block, func, max_length);
		}

		if(isJmpDst[offset]){
			src++; start_new_mapping(); src--;
		}

		//ppc_block->code_addr[offset] = dst;
		convert();
	}

	// Flush any remaining mapped registers
	flushRegisters(); //start_new_mapping();
	// In case we couldn't compile the whole function, use a pad
	if(need_pad){
		if(code_length + 8 > max_length)
			resizeCode(ppc_block, func, code_length + 8);
		genJumpPad();
	}

	// Here we recompute jumps and branches
	pass2(ppc_block);
	// Resize the code to exactly what is needed
	// FIXME: This doesn't work for some reason
	//resizeCode(ppc_block, *code_length);

	// Since this is a fresh block of code,
	// Make sure it wil show up in the ICache
	DCFlushRange(func->code, code_length*sizeof(PowerPC_instr));
	ICInvalidateRange(func->code, code_length*sizeof(PowerPC_instr));

	return func;
}

void init_block(MIPS_instr* mips_code, PowerPC_block* ppc_block){
	unsigned int length = (ppc_block->end_address - ppc_block->start_address)/sizeof(MIPS_instr);

	/*if(!ppc_block->code_addr){
		ppc_block->code_addr = malloc(length * sizeof(PowerPC_instr*));
		memset(ppc_block->code_addr, 0, length * sizeof(PowerPC_instr*));
	}*/
	ppc_block->mips_code = mips_code;

	// FIXME: Equivalent addresses should point to the same code/funcs?
	if(ppc_block->end_address < 0x80000000 || ppc_block->start_address >= 0xc0000000){
		unsigned long paddr;

		paddr = virtual_to_physical_address(ppc_block->start_address, 2);
		invalid_code_set(paddr>>12, 0);
		if(!blocks[paddr>>12]){
		     blocks[paddr>>12] = malloc(sizeof(PowerPC_block));
		     //blocks[paddr>>12]->code_addr = ppc_block->code_addr;
		     blocks[paddr>>12]->funcs = NULL;
		     blocks[paddr>>12]->start_address = paddr & ~0xFFF;
		     blocks[paddr>>12]->end_address = (paddr & ~0xFFF) + 0x1000;
		}

		paddr += ppc_block->end_address - ppc_block->start_address - 4;
		invalid_code_set(paddr>>12, 0);
		if(!blocks[paddr>>12]){
		     blocks[paddr>>12] = malloc(sizeof(PowerPC_block));
		     //blocks[paddr>>12]->code_addr = ppc_block->code_addr;
		     blocks[paddr>>12]->funcs = NULL;
		     blocks[paddr>>12]->start_address = paddr & ~0xFFF;
		     blocks[paddr>>12]->end_address = (paddr & ~0xFFF) + 0x1000;
		}

	} else {
		unsigned int start = ppc_block->start_address;
		unsigned int end   = ppc_block->end_address;
		if(start >= 0x80000000 && end < 0xa0000000 &&
		   invalid_code_get((start+0x20000000)>>12)){
			invalid_code_set((start+0x20000000)>>12, 0);
			if(!blocks[(start+0x20000000)>>12]){
				blocks[(start+0x20000000)>>12] = malloc(sizeof(PowerPC_block));
				//blocks[(start+0x20000000)>>12]->code_addr = ppc_block->code_addr;
				blocks[(start+0x20000000)>>12]->funcs = NULL;
				blocks[(start+0x20000000)>>12]->start_address
					= (start+0x20000000) & ~0xFFF;
				blocks[(start+0x20000000)>>12]->end_address
					= ((start+0x20000000) & ~0xFFF) + 0x1000;
			}
		}
		if(start >= 0xa0000000 && end < 0xc0000000 &&
		   invalid_code_get((start-0x20000000)>>12)){
			invalid_code_set((start-0x20000000)>>12, 0);
			if (!blocks[(start-0x20000000)>>12]){
				blocks[(start-0x20000000)>>12] = malloc(sizeof(PowerPC_block));
				//blocks[(start-0x20000000)>>12]->code_addr = ppc_block->code_addr;
				blocks[(start-0x20000000)>>12]->funcs = NULL;
				blocks[(start-0x20000000)>>12]->start_address
					= (start-0x20000000) & ~0xFFF;
				blocks[(start-0x20000000)>>12]->end_address
					= ((start-0x20000000) & ~0xFFF) + 0x1000;
			}
		}
	}
	invalid_code_set(ppc_block->start_address>>12, 0);
}

void deinit_block(PowerPC_block* ppc_block){
	invalidate_block(ppc_block);
	/*if(ppc_block->code_addr){
		invalidate_block(ppc_block);
		free(ppc_block->code_addr);
		ppc_block->code_addr = NULL;
	}*/
	invalid_code_set(ppc_block->start_address>>12, 1);

	// We need to mark all equivalent addresses as invalid
	if(ppc_block->end_address < 0x80000000 || ppc_block->start_address >= 0xc0000000){
		unsigned long paddr;

		paddr = virtual_to_physical_address(ppc_block->start_address, 2);
		if(blocks[paddr>>12]){
		     //blocks[paddr>>12]->code_addr = NULL;
		     invalid_code_set(paddr>>12, 1);
		}

		paddr += ppc_block->end_address - ppc_block->start_address - 4;
		if(blocks[paddr>>12]){
		     //blocks[paddr>>12]->code_addr = NULL;
		     invalid_code_set(paddr>>12, 1);
		}

	} else {
		unsigned int start = ppc_block->start_address;
		unsigned int end   = ppc_block->end_address;
		if(start >= 0x80000000 && end < 0xa0000000 &&
		   blocks[(start+0x20000000)>>12]){
			//blocks[(start+0x20000000)>>12]->code_addr = NULL;
			invalid_code_set((start+0x20000000)>>12, 1);
		}
		if(start >= 0xa0000000 && end < 0xc0000000 &&
		   blocks[(start-0x20000000)>>12]){
			//blocks[(start-0x20000000)>>12]->code_addr = NULL;
			invalid_code_set((start-0x20000000)>>12, 1);
		}
	}
}

int is_j_out(int branch, int is_aa){
	if(is_aa)
		return ((branch << 2 | (addr_first & 0xF0000000)) <  addr_first ||
		        (branch << 2 | (addr_first & 0xF0000000)) >= addr_last);
	else {
		int dst_instr = (src-1 - src_first) + branch;
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

		if(jump_table[i].type & JUMP_TYPE_CALL){ // Call to C function code
			// old_jump is the address of the function to call
			int jump_offset = ((unsigned int)jump_table[i].old_jump -
			                   (unsigned int)current)/4;
			// We're filling in a jump instrucion
			*current &= ~(PPC_LI_MASK << PPC_LI_SHIFT);
			PPC_SET_LI(*current, jump_offset);

		} else if(!(jump_table[i].type & JUMP_TYPE_J)){ // Branch instruction
			int jump_offset = (unsigned int)jump_table[i].old_jump +
				         ((unsigned int)jump_table[i].src_instr - (unsigned int)src_first)/4;

			jump_table[i].new_jump = code_addr[jump_offset] - current;

#if 0
			// FIXME: Reenable this when blocks are small enough to BC within
			//          Make sure that branch is using BC/B as appropriate
			*current &= ~(PPC_BD_MASK << PPC_BD_SHIFT);
			PPC_SET_BD(*current, jump_table[i].new_jump);
#else
			*current &= ~(PPC_LI_MASK << PPC_LI_SHIFT);
			PPC_SET_LI(*current, jump_table[i].new_jump);
#endif

		} else { // Jump instruction
			// The destination is actually calculated from the delay slot
			unsigned int jump_addr = (jump_table[i].old_jump << 2) |
			                         (ppc_block->start_address & 0xF0000000);

			// We're jumping within this block, find out where
			int jump_offset = (jump_addr - addr_first) >> 2;
			jump_table[i].new_jump = code_addr[jump_offset] - current;

			*current &= ~(PPC_LI_MASK << PPC_LI_SHIFT);
			PPC_SET_LI(*current, jump_table[i].new_jump);

		}
	}
}

static int pass0(PowerPC_block* ppc_block){
	// Scan over the MIPS code for this function and detect any
	//   any local branches and mark their destinations (for flushing).
	//   Determine src_last and addr_last by the end of the block
	//   (if reached) or the end of the function (exclusive of the
	//   delay slot as it will be recompiled ahead of the J/JR and
	//   does not need to be recompiled after the J/JR as it belongs
	//   to the next function if its used in that way.
	// If the function continues into the next block, we'll need to use
	//   a jump pad at the end of the block.
	unsigned int pc = addr_first >> 2;
	int i;
	// Set this to the end address of the block for is_j_out
	addr_last = ppc_block->end_address;
	// Zero out the jump destinations table
	for(i=0; i<1024; ++i) isJmpDst[i] = 0;
	// Go through each instruction and map every branch instruction's destination
	for(src = src_first; (pc < addr_last >> 2); ++src, ++pc){
		int opcode = MIPS_GET_OPCODE(*src);
		int index = pc - (ppc_block->start_address >> 2);
		if(opcode == MIPS_OPCODE_J || opcode == MIPS_OPCODE_JAL){
			unsigned int li = MIPS_GET_LI(*src);
			src+=2; ++pc;
			if(!is_j_out(li, 1)){
				assert( ((li&0x3FF) >= 0) && ((li&0x3FF) < 1024) );
				isJmpDst[ li & 0x3FF ] = 1;
			}
			--src;
			if(opcode == MIPS_OPCODE_JAL && index + 2 < 1024)
				isJmpDst[ index + 2 ] = 1;
			if(opcode == MIPS_OPCODE_J){ ++src, ++pc; break; }
		} else if(opcode == MIPS_OPCODE_BEQ   ||
		          opcode == MIPS_OPCODE_BNE   ||
		          opcode == MIPS_OPCODE_BLEZ  ||
		          opcode == MIPS_OPCODE_BGTZ  ||
		          opcode == MIPS_OPCODE_BEQL  ||
		          opcode == MIPS_OPCODE_BNEL  ||
		          opcode == MIPS_OPCODE_BLEZL ||
		          opcode == MIPS_OPCODE_BGTZL ||
		          opcode == MIPS_OPCODE_B     ||
		          (opcode == MIPS_OPCODE_COP1 &&
		           MIPS_GET_RS(*src) == MIPS_FRMT_BC)){
			int bd = MIPS_GET_IMMED(*src);
			src+=2; ++pc;
			bd |= (bd & 0x8000) ? 0xFFFF0000 : 0; // sign extend
			if(!is_j_out(bd, 0)){
				assert( index + 1 + bd >= 0 && index + 1 + bd < 1024 );
				isJmpDst[ index + 1 + bd ] = 1;
			}
			--src;
			if(index + 2 < 1024)
				isJmpDst[ index + 2 ] = 1;
		} else if(opcode == MIPS_OPCODE_R &&
		          (MIPS_GET_FUNC(*src) == MIPS_FUNC_JR ||
		           MIPS_GET_FUNC(*src) == MIPS_FUNC_JALR)){
			src+=2, pc+=2;
			break;
		} else if(opcode == MIPS_OPCODE_COP0 &&
		          MIPS_GET_FUNC(*src) == MIPS_FUNC_ERET){
			++src, ++pc;
			break;
		}
	}
	if(pc < addr_last >> 2){
		src_last = src;
		addr_last = pc << 2;
		return 0;
	} else {
		src_last = ppc_block->mips_code + 1024;
		addr_last = ppc_block->end_address;
		return 1;
	}
}

extern int stop;
inline unsigned long update_invalid_addr(unsigned long addr);
void jump_to(unsigned int address){
	stop = 1;
}
extern unsigned long jump_to_address;
void dyna_jump(){ jump_to(jump_to_address); }
void dyna_stop(){ }
void jump_to_func(){ jump_to(jump_to_address); }

// Warning: This is a slow operation, try not to use it
int resizeCode(PowerPC_block* block, PowerPC_func* func, int newSize){
	if(!func->code) return 0;

	// Creating the new block and copying the code
	PowerPC_instr* oldCode = func->code;
#ifdef USE_RECOMP_CACHE
	RecompCache_Realloc(func, newSize * sizeof(PowerPC_instr));
#else
	func->code = realloc(func->code, newSize * sizeof(PowerPC_instr));
#endif
	if(!func->code){ printf("Realloc failed. Panic!\n"); return 0; }

	if(func->code == oldCode) return newSize;

	// Readjusting pointers
	dst = func->code + (dst - oldCode);
	int i, start = (func->start_addr&0xfff)>>2;
	for(i=0; i<src-src_first; ++i)
		if(code_addr[i])
			code_addr[i] = func->code + (code_addr[i] - oldCode);
	for(i=0; i<current_jump; ++i)
		jump_table[i].dst_instr = func->code + (jump_table[i].dst_instr - oldCode);

	return newSize;
}

static void genJumpPad(void){
	PowerPC_instr ppc = NEW_PPC_INSTR();

	// noCheckInterrupt = 1
	GEN_LIS(ppc, 3, (unsigned int)(&noCheckInterrupt)>>16);
	set_next_dst(ppc);
	GEN_ORI(ppc, 3, 3, (unsigned int)(&noCheckInterrupt));
	set_next_dst(ppc);
	GEN_LI(ppc, 0, 0, 1);
	set_next_dst(ppc);
	GEN_STW(ppc, 0, 0, 3);
	set_next_dst(ppc);

	// Set the next address to the first address in the next block if
	//   we've really reached the end of the block, not jumped to the pad
	GEN_LIS(ppc, 3, (get_src_pc()+4)>>16);
	set_next_dst(ppc);
	GEN_ORI(ppc, 3, 3, get_src_pc()+4);
	set_next_dst(ppc);

	// return destination
	GEN_BLR(ppc,0);
	set_next_dst(ppc);
}

void invalidate_block(PowerPC_block* ppc_block){
	// Free the code for all the functions in this block
#if 0
	PowerPC_func_node* n, * next;
	for(n=ppc_block->funcs; n != NULL; n = next){
		next = n->next;
#ifdef USE_RECOMP_CACHE
		RecompCache_Free(ppc_block->start_address | n->function->start_addr);
#else
		free(n->function->code);
		free(n->function->code_addr);
		free(n->function);
		free(n);
#endif
	}
	ppc_block->funcs = NULL;
#else
	PowerPC_func_node* free_tree(PowerPC_func_node* node){
		if(!node) return NULL;
		node->left = free_tree(node->left);
		node->right = free_tree(node->right);
		RecompCache_Free(ppc_block->start_address | node->function->start_addr);
		return NULL;
	}
	ppc_block->funcs = free_tree(ppc_block->funcs);
#endif
	// NULL out code_addr
	//memset(ppc_block->code_addr, 0, 1024 * sizeof(PowerPC_instr*));
	// Now that we've handled the invalidation, reinit ourselves
	init_block(ppc_block->mips_code, ppc_block);
}

