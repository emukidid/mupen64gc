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
#define DYNAREG_RDRAM  19
#define DYNAREG_FPR    20

#define DYNAFRAME_SIZE 40
#define DYNAOFF_LR     (DYNAFRAME_SIZE+4)
#define DYNAOFF_CR     8
#define DYNAOFF_REG    12
#define DYNAOFF_ZERO   16
#define DYNAOFF_INTERP 20
#define DYNAOFF_UCOUNT 24
#define DYNAOFF_LADDR  28
#define DYNAOFF_RDRAM  32
#define DYNAOFF_FPR    36


extern long long int reg[34]; // game's registers
extern long long int reg_cop1_fgr_64[32]; // fprs

extern int noCheckInterrupt;

unsigned int decodeNInterpret(MIPS_instr, unsigned int, int);
int dyna_update_count(unsigned int pc);

#define Count reg_cop0[9]

#endif

