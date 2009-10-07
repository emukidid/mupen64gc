/* TLB-Cache-hash.c - This is how the TLB LUT should be accessed, using a hash map
   by Mike Slegeir for Mupen64-GC
   ----------------------------------------------------
   FIXME: This should be profiled to determine the best size,
            currently, the linked lists' length can be up to ~16,000
   ----------------------------------------------------
   MEMORY USAGE:
     STATIC:
     	TLB LUT r: NUM_SLOTS * 4 (currently 256 bytes)
     	TLB LUT w: NUM_SLOTS * 4 (currently 256 bytes)
     HEAP:
     	TLB hash nodes: 2 (r/w) * 12 bytes * O( 2^20 ) entries
 */

#include <stdlib.h>
#include "TLB-Cache.h"
#include "../gui/DEBUG.h"

#ifdef USE_TLB_CACHE

// Num Slots must be a power of 2!
#define TLB_NUM_SLOTS 64
// The amount of bits required to represent a page
//   number, don't change. Required for hash calc
#define TLB_BITS_PER_PAGE_NUM 20

typedef struct node {
	unsigned int value;
	unsigned int page;
	struct node* next;
} TLB_hash_node;

static TLB_hash_node* TLB_LUT_r[TLB_NUM_SLOTS];
static TLB_hash_node* TLB_LUT_w[TLB_NUM_SLOTS];

static unsigned int TLB_hash_mask;

void TLBCache_init(void){
	TLB_hash_mask = 0;
	unsigned int temp = TLB_NUM_SLOTS, temp2 = 1;
	while(temp){
		temp >>= 1;
		TLB_hash_mask |= temp2;
		temp2 <<= 1;
	}
}

void TLBCache_deinit(void){
	int i;
	TLB_hash_node* node, * next;
	for(i=0; i<TLB_NUM_SLOTS; ++i){
		// Clear r
		for(node = TLB_LUT_r[i]; node != NULL; node = next){
			next = node->next;
			free(node);
		}
		TLB_LUT_r[i] = NULL;
		
		// Clear w
		for(node = TLB_LUT_w[i]; node != NULL; node = next){
			next = node->next;
			free(node);
		}
		TLB_LUT_w[i] = NULL;
	}
}

static unsigned int inline TLB_hash(unsigned int page){
	return page & TLB_hash_mask;
}

unsigned int inline TLBCache_get_r(unsigned int page){
#ifdef DEBUG_TLB
	sprintf(txtbuffer, "TLB r read %05x\n", page);
	DEBUG_print(txtbuffer, DBG_USBGECKO);
#endif
	TLB_hash_node* node = TLB_LUT_r[ TLB_hash(page) ];
	
	for(; node != NULL; node = node->next)
		if(node->page == page) return node->value;
	
	return 0;
}

unsigned int inline TLBCache_get_w(unsigned int page){
#ifdef DEBUG_TLB
	sprintf(txtbuffer, "TLB w read %05x\n", page);
	DEBUG_print(txtbuffer, DBG_USBGECKO);
#endif
	TLB_hash_node* node = TLB_LUT_w[ TLB_hash(page) ];
	
	for(; node != NULL; node = node->next)
		if(node->page == page) return node->value;
	
	return 0;
}

void inline TLBCache_set_r(unsigned int page, unsigned int val){
#ifdef DEBUG_TLB
	sprintf(txtbuffer, "TLB r write %05x %08x\n", page, val);
	DEBUG_print(txtbuffer, DBG_USBGECKO);
#endif
	TLB_hash_node* next = TLB_LUT_r[ TLB_hash(page) ];
	
	TLB_hash_node* node = malloc( sizeof(TLB_hash_node) );
	node->page  = page;
	node->value = val;
	node->next  = next;
	TLB_LUT_r[ TLB_hash(page) ] = node;
	
	// Remove any old values for this page from the linked list
	for(; node != NULL; node = node->next)
		if(node->next != NULL && node->next->page == page){
			TLB_hash_node* old_node = node->next;
			node->next = old_node->next;
			free( old_node );
			break;
		}
}

void inline TLBCache_set_w(unsigned int page, unsigned int val){
#ifdef DEBUG_TLB
	sprintf(txtbuffer, "TLB r write %05x %08x\n", page, val);
	DEBUG_print(txtbuffer, DBG_USBGECKO);
#endif
	TLB_hash_node* next = TLB_LUT_w[ TLB_hash(page) ];
	
	TLB_hash_node* node = malloc( sizeof(TLB_hash_node) );
	node->page  = page;
	node->value = val;
	node->next  = next;
	TLB_LUT_w[ TLB_hash(page) ] = node;
	
	// Remove any old values for this page from the linked list
	for(; node != NULL; node = node->next)
		if(node->next != NULL && node->next->page == page){
			TLB_hash_node* old_node = node->next;
			node->next = old_node->next;
			free( old_node );
			break;
		}
}

char* TLBCache_dump(){
	int i;
	DEBUG_print("\n\nTLB Cache r dump:\n", DBG_USBGECKO);
	for(i=0; i<TLB_NUM_SLOTS; ++i){
		sprintf(txtbuffer, "%d\t", i);
		DEBUG_print(txtbuffer, DBG_USBGECKO);
		TLB_hash_node* node = TLB_LUT_r[i];
		for(; node != NULL; node = node->next){
			sprintf(txtbuffer, "%05x,%05x -> ", node->page, node->value);
			DEBUG_print(txtbuffer, DBG_USBGECKO);
		}
		DEBUG_print("\n", DBG_USBGECKO);
	}
	DEBUG_print("\n\nTLB Cache w dump:\n", DBG_USBGECKO);
	for(i=0; i<TLB_NUM_SLOTS; ++i){
		sprintf(txtbuffer, "%d\t", i);
		DEBUG_print(txtbuffer, DBG_USBGECKO);
		TLB_hash_node* node = TLB_LUT_w[i];
		for(; node != NULL; node = node->next){
			sprintf(txtbuffer, "%05x,%05x -> ", node->page, node->value);
			DEBUG_print(txtbuffer, DBG_USBGECKO);
		}
		DEBUG_print("\n", DBG_USBGECKO);
	}
	return "TLB Cache dumped to USB Gecko";
}

#endif
