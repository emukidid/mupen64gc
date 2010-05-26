/* FuncTree.c - Handles the BST for functions within a block
 * by Mike Slegeir for Mupen64-GC
 */

#include <stdlib.h>
#include "../r4300.h"
#include "Recompile.h"
#include "../Recomp-Cache.h"

static inline PowerPC_func_node** _find(PowerPC_func_node** node, unsigned int addr){
	while(*node){
		if(addr < (*node)->function->start_addr)
			node = &(*node)->left;
		else if(addr >= (*node)->function->end_addr)
			node = &(*node)->right;
		else
			break;
	}
	return node;
}

PowerPC_func* find_func(PowerPC_func_node** root, unsigned int addr){
	start_section(FUNCS_SECTION);
	PowerPC_func_node* node = *_find(root, addr);
	end_section(FUNCS_SECTION);
	return node ? node->function : NULL;
}

void insert_func(PowerPC_func_node** root, PowerPC_func* func){
	PowerPC_func_node** node = _find(root, func->start_addr);
	if(*node) return; // Avoid a memory leak if this function exists

	*node = MetaCache_Alloc(sizeof(PowerPC_func_node));
	(*node)->function = func;
	(*node)->left = (*node)->right = NULL;
}

void remove_func(PowerPC_func_node** root, PowerPC_func* func){
	PowerPC_func_node** node = _find(root, func->start_addr);
	if(!*node) return; // Avoid a memory error if the function doesn't exist

	PowerPC_func_node* old = *node;
	if(!(*node)->left)
		*node = (*node)->right;
	else if(!(*node)->right)
		*node = (*node)->left;
	else {
		// The node has two children, find the node's predecessor and swap
		PowerPC_func_node** pre;
		for(pre = &(*node)->left; (*pre)->right; pre = &(*pre)->right);
		(*node)->function = (*pre)->function;
		old = *pre;
		*pre = (*pre)->left;
	}

	MetaCache_Free(old);
}

