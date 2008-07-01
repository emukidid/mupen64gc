/* FFI.c: Scheme Dynarec <-> C Emulator interface
   by Mike Slegeir for Mupen64-GC
 */

#include <stdio.h>
#include "/home/mike/tmp/mupen64_scheme/memory/memory.h"
#include "/home/mike/tmp/mupen64_scheme/r4300/r4300.h"
#include "scheme.h"
#include "FFI.h"

extern void (*interp_ops[64])(void);
extern char* MIPS_scheme_code;
extern char* Recompile_scheme_code;

#ifdef MZ_SCHEME
static Scheme_Env* env;
#endif

//unsigned int decodeNInterpret(unsigned int instr, unsigned int addr){
#ifdef MZ_SCHEME
Scheme_Object* decodeNInterpret(int argc, Scheme_Object* argv[]){
	unsigned int instr;
	SCM2UINT(argv[0], instr);
	SCM2UINT(argv[1], interp_addr);
#endif
	
	printf("Interpreting %08x @ %08x\n", instr, interp_addr);
	prefetch_opcode(instr);
	interp_ops[((instr >> 26) & 0x3F)]();
	
	return UINT2SCM(interp_addr);
}

// int isStopSignaled(void){
#ifdef MZ_SCHEME
Scheme_Object* isStopSignaled(int argc, Scheme_Object* argv[]){
#endif
	return stop ? scheme_true : scheme_false;
}

//long regLoad(int which){ return (long)reg[which]; }
#ifdef MZ_SCHEME
Scheme_Object* regLoad(int argc, Scheme_Object* argv[]){
	int which;
	SCM2INT(argv[0], which);
	return INT2SCM((int)reg[which]);
}
#endif
//unsigned long uregLoad(int which){ return (unsigned long)reg[which]; }
#ifdef MZ_SCHEME
Scheme_Object* uregLoad(int argc, Scheme_Object* argv[]){
	int which;
	SCM2INT(argv[0], which);
	return UINT2SCM((unsigned int)reg[which]);
}
#endif
//long reg64Load(int which){ return (long long)reg[which]; }
#ifdef MZ_SCHEME
Scheme_Object* reg64Load(int argc, Scheme_Object* argv[]){
	int which;
	SCM2INT(argv[0], which);
	return LONG2SCM((long long)reg[which]);
}
#endif
//unsigned long long ureg64Load(int which){ return (unsigned long long)reg[which]; }
#ifdef MZ_SCHEME
Scheme_Object* ureg64Load(int argc, Scheme_Object* argv[]){
	int which;
	SCM2INT(argv[0], which);
	return ULONG2SCM((unsigned long long)reg[which]);
}
#endif

//void regStore(int which, long val){ reg[which] = val; }
#ifdef MZ_SCHEME
Scheme_Object* regStore(int argc, Scheme_Object* argv[]){
	int which;
	SCM2INT(argv[0], which);
	int val;
	SCM2INT(argv[1], val);
	reg[which] = (long long)val;
	return scheme_false;
}
#endif
//void uregStore(int which, unsigned long val){ reg[which] = val; }
#ifdef MZ_SCHEME
Scheme_Object* uregStore(int argc, Scheme_Object* argv[]){
	int which;
	SCM2INT(argv[0], which);
	unsigned int val;
	SCM2UINT(argv[1], val);
	reg[which] = (unsigned long long)val;
	return scheme_false;
}
#endif
//void reg64Store(int which, long long val){ reg[which] = val; }
#ifdef MZ_SCHEME
Scheme_Object* reg64Store(int argc, Scheme_Object* argv[]){
	int which;
	SCM2INT(argv[0], which);
	long long val;
	SCM2LONG(argv[1], val);
	reg[which] = (long long)val;
	return scheme_false;
}
#endif
//void ureg64Store(int which, unsigned long long val){ reg[which] = val; }
#ifdef MZ_SCHEME
Scheme_Object* ureg64Store(int argc, Scheme_Object* argv[]){
	int which;
	SCM2INT(argv[0], which);
	unsigned long long val;
	SCM2ULONG(argv[1], val);
	reg[which] = (unsigned long long)val;
	return scheme_false;
}
#endif

//unsigned int umemLoad(unsigned int addr){
#ifdef MZ_SCHEME
Scheme_Object* fetch(int argc, Scheme_Object* argv[]){
#endif
	unsigned int addr;
	SCM2UINT(argv[0], addr);
	printf("fetching instruction @ %08x\n", addr);
	unsigned int op = -1;
	
	if((addr >= 0x80000000) && (addr < 0xc0000000)){
		if((addr >= 0x80000000) && (addr < 0x80800000)){
			op = rdram[(addr&0xFFFFFF)/4];
		} else if((addr >= 0xa4000000) && (addr < 0xa4001000)){
			op = SP_DMEM[(addr&0xFFF)/4];
		} else if((addr > 0xb0000000)){
			op = ((unsigned long*)rom)[(addr & 0xFFFFFFF)/4];
		} else {
			stop = 1;
			*((int*)0) = 0xDEADBEEF;
		}
	} else {
		// TODO: Virtual addresses
		stop = 1;
		*((int*)0) = 0x1337;
	}
	
	return UINT2SCM(op);
}

// TODO: All of this
long memLoad(unsigned int addr){
	address = addr;
	read_word_in_memory();
	return *(long*)rdword;
}

signed char mem8Load(unsigned int addr){
	address = addr;
	read_byte_in_memory();
	return *(signed char*)rdword;
}

short mem16Load(unsigned int addr){
	address = addr;
	read_hword_in_memory();
	return *(short*)rdword;
}

long long mem64Load(unsigned int addr){
	address = addr;
	read_dword_in_memory();
	return *rdword;
}

void memStore(unsigned int addr, long val){
	address = addr;
	*(long*)rdword = val;
	write_word_in_memory();
}

void mem8Store(unsigned int addr, signed char val){
	address = addr;
	*(signed char*)rdword = val;
	write_byte_in_memory();
}

void mem16Store(unsigned int addr, short val){
	address = addr;
	*(short*)rdword = val;
	write_hword_in_memory();
}

void mem64Store(unsigned int addr, long long val){
	address = addr;
	*rdword = val;
	write_dword_in_memory();
}

void dynaInvalidate(unsigned int start, unsigned int stop){
	char call[64];
	sprintf(call, "(invalidate-range #x%08x #x%08x)", start, stop);
	scheme_eval_string(call, env);
}

#ifdef MZ_SCHEME
#include "required.c" // Ugly huh? Seems like the best way though

static int setup_mzscheme(Scheme_Env* e, int argc, char* argv[]){
	int i;
    mz_jmp_buf * volatile save, fresh;
    
    /* Declare embedded modules in "required.c": */
    printf("declaring modules\n");
    declare_modules(e);
    
    printf("requiring mzscheme\n");
    scheme_namespace_require(scheme_intern_symbol("mzscheme"));
    printf("requiring mips/Recompile.ss\n");
    scheme_namespace_require(scheme_intern_symbol("mips/Recompile"));
    
    // Create a Scheme function for each C function that needs to be called
    printf("registering functions\n");
    SCM_DEFUN(e, decodeNInterpret, "decode&interpret", 2, 2);
    Scheme_Object* module = scheme_intern_symbol("mips/Bridge");
    SCM_DEFUN_MOD(e, module, isStopSignaled, "stop-signaled?", 0, 0);
    SCM_DEFUN_MOD(e, module, fetch, "fetch", 1, 1);
    SCM_DEFUN(e, regLoad, "reg", 1, 1);
    SCM_DEFUN(e, uregLoad, "ureg", 1, 1);
    SCM_DEFUN(e, reg64Load, "reg64", 1, 1);
    SCM_DEFUN(e, ureg64Load, "ureg64", 1, 1);
    SCM_DEFUN(e, regStore, "r=", 2, 2);
    SCM_DEFUN(e, uregStore, "ur=", 2, 2);
    SCM_DEFUN(e, reg64Store, "r64=", 2, 2);
    SCM_DEFUN(e, ureg64Store, "ur64=", 2, 2);
	
#if 0 
	save = scheme_current_thread->error_buf;
    scheme_current_thread->error_buf = &fresh;
    
    printf("setjmp\n");
	if(scheme_setjmp(scheme_error_buf)){
		// This block will only be executed after a longjmp (an error in scheme code)
		scheme_current_thread->error_buf = save;
		env = NULL;
		printf("scheme exception caught\n");
		return -1; /* There was an error */
	
	} else {
		// This block of code is executed immediately after setjmp
		// Load our scheme source
		printf("loading scheme code\n");
		Scheme_Object* v;
		printf("loading MIPS.ss with contents:\n%s\n", &MIPS_scheme_code);
		scheme_eval_string_all(&MIPS_scheme_code, e, 1);
		printf("loading Recompile.ss with contents:\n%s\n", &Recompile_scheme_code);
		scheme_eval_string_all(&Recompile_scheme_code, e, 1);
		fflush(stdout);
		scheme_current_thread->error_buf = save;
	}
#endif
	
	env = e;
	return 0;
}
#endif // MZ_SCHEME

int init_scheme_dynarec(void){
	printf("initing scheme\n");
#ifdef MZ_SCHEME
	return scheme_main_setup(1, setup_mzscheme, 0, NULL);
#endif // MZ_SCHEME
}

unsigned int dynarec(unsigned int addr){
	char call[64];
	unsigned int ret;
	sprintf(call, "(dynarec #x%08x)", addr);
	dynacore = 0;
	interpcore = 1;
	last_addr = addr;
	
	mz_jmp_buf * volatile save, fresh;
	save = scheme_current_thread->error_buf;
    scheme_current_thread->error_buf = &fresh;
    
    if(scheme_setjmp(scheme_error_buf)){
		// This block will only be executed after a longjmp (an error in scheme code)
		scheme_current_thread->error_buf = save;
		env = NULL;
		printf("scheme exception caught\n");
		ret = -1; /* There was an error */
    } else {
		// This block of code is executed immediately after setjmp
		printf("I'm going in: %s\n", call);
		SCM2UINT( scheme_eval_string(call, env), ret );
		scheme_current_thread->error_buf = save;
    }
	
	dynacore = 1;
	interpcore = 0;
	return ret;
}
