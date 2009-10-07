/* Wrappers.h - Wrappers for recompiled code, these start and return from N64 code
   by Mike Slegeir for Mupen64-GC
 */

#ifndef WRAPPERS_H
#define WRAPPERS_H

#include "Recompile.h"

#define DYNAREG_REG    14
#define DYNAREG_ZERO   15
#define DYNAREG_INTERP 16
#define DYNAREG_UCOUNT 17
#define DYNAREG_LADDR  18

#define DYNAOFF_LR     36 // This is weird, but it belongs in the last frame
#define DYNAOFF_CR     8
#define DYNAOFF_REG    12
#define DYNAOFF_ZERO   16
#define DYNAOFF_INTERP 20
#define DYNAOFF_UCOUNT 24
#define DYNAOFF_LADDR  28

extern long long int reg[34]; // game's registers
extern long long int reg_cop1_fgr_64[32];

unsigned int decodeNInterpret(MIPS_instr, unsigned int, int);
int dyna_update_count(unsigned int pc);

#define Count reg_cop0[9]

#endif

