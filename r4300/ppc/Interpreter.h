/* Interpreter.h - Headers and defines for calling the interpreter
   by Mike Slegeir for Mupen64-GC
 */

#ifndef INTERPRETER_H
#define INTERPRETER_H

/* decodeNInterpret(MIPS_instr, unsigned int PC):
	1) Saves state
	2) Calls prefetch_opcode(instr)               (Decodes)
	3) Calls interp_ops[MIPS_GET_OPCODE(instr)]() (Interprets)
	4) Restores state
 */
unsigned int decodeNInterpret();

// These defines tell the recompiler to interpret
//  rather than recompile the instruction
#define INTERPRET_LB
#define INTERPRET_LBU
#define INTERPRET_LH
#define INTERPRET_LHU
#define INTERPRET_LW
#define INTERPRET_LWL
#define INTERPRET_LWR

#define INTERPRET_SB
#define INTERPRET_SH
#define INTERPRET_SW
#define INTERPRET_SWL
#define INTERPRET_SWR

#define INTERPRET_DW

#define INTERPRET_MADD
#define INTERPRET_MADDU
#define INTERPRET_MSUB
#define INTERPRET_MSUBU

//#define INTERPRET_CLO

#define INTERPRET_HILO
#if 1
#define INTERPRET_MULT
#define INTERPRET_MULTU
#define INTERPRET_DIV
#define INTERPRET_DIVU
#endif

//#define INTERPRET_MOVN
//#define INTERPRET_MOVZ

//#define INTERPRET_J
//#define INTERPRET_JAL
#define INTERPRET_JR
#define INTERPRET_JALR
#define INTERPRET_BC
//#define INTERPRET_BRANCH

#define INTERPRET_SYSCALL
#define INTERPRET_BREAK
#define INTERPRET_TRAPS

#define INTERPRET_LL
#define INTERPRET_SC

#define INTERPRET_COP0

#define INTERPRET_FP

#endif

