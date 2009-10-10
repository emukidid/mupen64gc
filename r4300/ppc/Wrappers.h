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
#define DYNAREG_SPDMEM 20
#define DYNAREG_FPR_32 21
#define DYNAREG_FPR_64 22
#define DYNAREG_FCR31  23

#define DYNAOFF_LR     20

extern long long int reg[34]; // game's registers
extern float*  reg_cop1_simple[32]; // 32-bit fprs
extern double* reg_cop1_double[32]; // 64-bit fprs

extern int noCheckInterrupt;

unsigned int decodeNInterpret(MIPS_instr, unsigned int, int);
int dyna_update_count(unsigned int pc);

#define Count reg_cop0[9]

#endif

