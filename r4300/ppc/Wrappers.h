/* Wrappers.h - Wrappers for recompiled code, these start and return from N64 code
   by Mike Slegeir for Mupen64-GC
 */

#ifndef WRAPPERS_H
#define WRAPPERS_H

#include "Recompile.h"

extern long long int reg[34]; // game's registers
extern long long int reg_cop1_fgr_64[32];

unsigned int decodeNInterpret(MIPS_instr, unsigned int, unsigned int);

#endif

