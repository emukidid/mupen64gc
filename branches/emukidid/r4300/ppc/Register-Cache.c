/* Register-Cache.c - Handle mappings from MIPS to PPC registers
   by Mike Slegeir for Mupen64-GC
 */

#include "Register-Cache.h"
#include "PowerPC.h"
#include "Wrappers.h"

static struct {
	// Holds the value of the physical reg or -1 (hi, lo)
	RegMapping map;
	int dirty; // Nonzero means the register must be flushed to memory
	int lru;   // LRU value for flushing; higher is newer
} regMap[34];

static unsigned int nextLRUVal;
static int availableRegsDefault[32] = {
	0, /* r0 is mostly used for saving/restoring lr: used as a temp */
	0, /* sp: leave alone! */
	0, /* gp: leave alone! */
	1,1,1,1,1,1,1,1, /* Volatile argument registers */
	0,0, /* Volatile registers used for special purposes: dunno */
	/* Non-volatile registers: using might be too costly */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	};
static int availableRegs[32];

// Actually perform the store for a dirty register mapping
static void flushRegister(int reg){
	PowerPC_instr ppc;
	if(regMap[reg].map.hi >= 0){
		// Simply store the mapped MSW
		GEN_STW(ppc, regMap[reg].map.hi, reg*8, DYNAREG_REG);
		set_next_dst(ppc);
	} else {
		// Sign extend to 64-bits
		GEN_SRAWI(ppc, 0, regMap[reg].map.lo, 31);
		set_next_dst(ppc);
		// Store the MSW
		GEN_STW(ppc, 0, reg*8, DYNAREG_REG);
		set_next_dst(ppc);
	}
	// Store the LSW
	GEN_STW(ppc, regMap[reg].map.lo, reg*8+4, DYNAREG_REG);
	set_next_dst(ppc);
}
// Find an available HW reg or -1 for none
static int getAvailableHWReg(void){
	int i;
	// Iterate over the HW registers and find one that's available
	for(i=0; i<32; ++i){
		if(availableRegs[i]){
			availableRegs[i] = 0;
			return i;
		}
	}
	return -1;
}

int flushRegisters(void){
	int i, flushed = 0;
	for(i=1; i<34; ++i){
		if(regMap[i].map.lo >= 0 && regMap[i].dirty){
			flushRegister(i);
			++flushed;
		}
		// Mark unmapped
		regMap[i].map.hi = regMap[i].map.lo = -1;
	}
	memcpy(availableRegs, availableRegsDefault, 32*sizeof(int));
	nextLRUVal = 0;
	return flushed;
}

static RegMapping flushLRURegister(void){
	int i, lru_i = 0, lru_v = 0x7fffffff;
	for(i=1; i<34; ++i){
		if(regMap[i].map.lo >= 0 && regMap[i].lru < lru_v){
			lru_i = i; lru_v = regMap[i].lru;
		}
	}
	RegMapping map = regMap[lru_i].map;
	// Flush the register if its dirty
	if(regMap[lru_i].dirty) flushRegister(lru_i);
	// Mark unmapped
	regMap[lru_i].map.hi = regMap[lru_i].map.lo = -1;
	return map;
}

void invalidateRegisters(void){
	int i;
	for(i=0; i<34; ++i)
		// Mark unmapped
		regMap[i].map.hi = regMap[i].map.lo = -1;
	memcpy(availableRegs, availableRegsDefault, 32*sizeof(int));
	nextLRUVal = 0;
}

int mapRegisterNew(int reg){
	if(!reg) return 0; // Discard any writes to r0
	regMap[reg].lru = nextLRUVal++;
	regMap[reg].dirty = 1; // Since we're writing to this reg, its dirty
	
	// If its already been mapped, just return that value
	if(regMap[reg].map.lo >= 0){
		// If the hi value is mapped, free the mapping
		if(regMap[reg].map.hi >= 0){
			availableRegs[regMap[reg].map.hi] = 1;
			regMap[reg].map.hi = -1;
		}
		return regMap[reg].map.lo;
	}
	
	// Try to find any already available register
	int available = getAvailableHWReg();
	if(available >= 0) return regMap[reg].map.lo = available;
	// We didn't find an available register, so flush one
	RegMapping lru = flushLRURegister();
	if(lru.hi >= 0) availableRegs[lru.hi] = 1;
	
	return regMap[reg].map.lo = lru.lo;
}

RegMapping mapRegister64New(int reg){
	if(!reg) return (RegMapping){ 0, 0 };
	regMap[reg].lru = nextLRUVal++;
	regMap[reg].dirty = 1; // Since we're writing to this reg, its dirty
	// If its already been mapped, just return that value
	if(regMap[reg].map.lo >= 0){
		// If the hi value is not mapped, find a mapping
		if(regMap[reg].map.hi < 0){
			// Try to find any already available register
			int available = getAvailableHWReg();
			if(available >= 0) regMap[reg].map.hi = available;
			else {
				// We didn't find an available register, so flush one
				RegMapping lru = flushLRURegister();
				if(lru.hi >= 0) availableRegs[lru.hi] = 1;
				regMap[reg].map.hi = lru.lo;
			}
		}
		// Return the mapping
		return regMap[reg].map;
	}
	
	// Try to find any already available registers
	regMap[reg].map.lo = getAvailableHWReg();
	regMap[reg].map.hi = getAvailableHWReg();
	// If there weren't enough registers, we'll have to flush
	if(regMap[reg].map.lo < 0){
		// We didn't find any available registers, so flush one
		RegMapping lru = flushLRURegister();
		if(lru.hi >= 0) regMap[reg].map.hi = lru.hi;
		regMap[reg].map.lo = lru.lo;
	}
	if(regMap[reg].map.hi < 0){
		// We didn't find an available register, so flush one
		RegMapping lru = flushLRURegister();
		if(lru.hi >= 0) availableRegs[lru.hi] = 1;
		regMap[reg].map.hi = lru.lo;
	}
	// Return the mapping
	return regMap[reg].map;
}

int mapRegister(int reg){
	PowerPC_instr ppc;
	if(!reg) return DYNAREG_ZERO; // Return r0 mapped to r14
	regMap[reg].lru = nextLRUVal++;
	// If its already been mapped, just return that value
	if(regMap[reg].map.lo >= 0){
		// If the hi value is mapped, free the mapping
		if(regMap[reg].map.hi >= 0){
			availableRegs[regMap[reg].map.hi] = 1;
			regMap[reg].map.hi = -1;
		}
		return regMap[reg].map.lo;
	}
	regMap[reg].dirty = 0; // If it hasn't previously been mapped, its clean
	int i;
	// Iterate over the HW registers and find one that's available
	for(i=0; i<32; ++i)
		if(availableRegs[i]){
			GEN_LWZ(ppc, i, reg*8+4, DYNAREG_REG);
			set_next_dst(ppc);
			
			availableRegs[i] = 0;
			return regMap[reg].map.lo = i;
		}
	// We didn't find an available register, so flush one
	RegMapping lru = flushLRURegister();
	if(lru.hi >= 0) availableRegs[lru.hi] = 1;
	// And load the registers value to the register we flushed
	GEN_LWZ(ppc, lru.lo, reg*8+4, DYNAREG_REG);
	set_next_dst(ppc);
	
	return regMap[reg].map.lo = lru.lo;
}

RegMapping mapRegister64(int reg){
	PowerPC_instr ppc;
	if(!reg) return (RegMapping){ DYNAREG_ZERO, DYNAREG_ZERO };
	regMap[reg].lru = nextLRUVal++;
	// If its already been mapped, just return that value
	if(regMap[reg].map.lo >= 0){
		// If the hi value is not mapped, find a mapping
		if(regMap[reg].map.hi < 0){
			// Try to find any already available register
			int available = getAvailableHWReg();
			if(available >= 0) regMap[reg].map.hi = available;
			else {
				// We didn't find an available register, so flush one
				RegMapping lru = flushLRURegister();
				if(lru.hi >= 0) availableRegs[lru.hi] = 1;
				regMap[reg].map.hi = lru.lo;
			}
			// Sign extend to 64-bits
			GEN_SRAWI(ppc, regMap[reg].map.hi, regMap[reg].map.lo, 31);
			set_next_dst(ppc);
		}
		// Return the mapping
		return regMap[reg].map;
	}
	
	// Try to find any already available registers
	regMap[reg].map.lo = getAvailableHWReg();
	regMap[reg].map.hi = getAvailableHWReg();
	// If there weren't enough registers, we'll have to flush
	if(regMap[reg].map.lo < 0){
		// We didn't find any available registers, so flush one
		RegMapping lru = flushLRURegister();
		if(lru.hi >= 0) regMap[reg].map.hi = lru.hi;
		regMap[reg].map.lo = lru.lo;
	}
	if(regMap[reg].map.hi < 0){
		// We didn't find an available register, so flush one
		RegMapping lru = flushLRURegister();
		if(lru.hi >= 0) availableRegs[lru.hi] = 1;
		regMap[reg].map.hi = lru.lo;
	}
	// Load the values into the registers
	GEN_LWZ(ppc, regMap[reg].map.hi, reg*8, DYNAREG_REG);
	set_next_dst(ppc);
	GEN_LWZ(ppc, regMap[reg].map.lo, reg*8+4, DYNAREG_REG);
	set_next_dst(ppc);
	// Return the mapping
	return regMap[reg].map;
}

int mapRegisterTemp(void){
	// Try to find an already available register
	int available = getAvailableHWReg();
	if(available >= 0) return available;
	// If there are none, flush the LRU and use it
	RegMapping lru = flushLRURegister();
	if(lru.hi >= 0) availableRegs[lru.hi] = 1;
	return lru.lo;
}

void unmapRegisterTemp(int reg){
	availableRegs[reg] = 1;
}

