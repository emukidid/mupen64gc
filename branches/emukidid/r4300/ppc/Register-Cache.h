/* Register-Cache.h - Handle mappings from MIPS to PPC registers
   by Mike Slegeir for Mupen64-GC
 */

#ifndef REGISTER_CACHE_H
#define REGISTER_CACHE_H

typedef struct { int hi, lo; } RegMapping;

// Unmap all registers, storing any dirty registers
int flushRegisters(void);
// Unmap all registers without storing any
void invalidateRegisters(void);
// Create a mapping for a 32-bit register (reg) to a HW register (returned)
// Loading the registers' value if the mapping doesn't already exist
int mapRegister(int reg);
// Create a mapping for a 32-bit register (reg) to a HW register (returned)
// Marking the mapping dirty so that it is stored when flushed
int mapRegisterNew(int reg);
// Create a mapping for a 64-bit register (reg) to 2 HW registers (returned)
// Loading the registers' value if the mapping doesn't already exist
RegMapping mapRegister64(int reg);
// Create a mapping for a 64-bit register (reg) to 2 HW registers (returned)
// Marking the mapping dirty so that it is stored when flushed
RegMapping mapRegister64New(int reg);
// Reserve a HW register to be used but not associated with any registers
// When the register is no longer needed, be sure to call unmapRegisterTemp
int mapRegisterTemp(void);
// Frees a previously reserved register
void unmapRegisterTemp(int tmp);

#endif
