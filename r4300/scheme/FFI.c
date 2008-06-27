/* FFI.c: Scheme Dynarec <-> C Emulator interface
   by Mike Slegeir for Mupen64-GC
 */

#include "../../memory/memory.h"
#include "../r4300.h"
#include "scheme.h"
#include "FFI.h"


//unsigned int decodeNInterpret(unsigned int instr, unsigned int addr){
#ifdef MZ_SCHEME
Scheme_Object* decodeNInterpret(int argc, Scheme_Object* argv[]){
	unsigned int instr;
	SCM2UINT(argv[0], insr);
	unsigned int addr;
	SCM2UINT(argv[0], addr);
#endif
	
	interp_addr = addr;
	prefetch_opcode(mips);
	interp_ops[MIPS_GET_OPCODE(mips)]();
	
	//return interp_addr != addr + 4 ? interp_addr : 0;
	addr = interp_addr != addr + 4 ? interp_addr : 0;
	return UINT2SCM(addr);
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
Scheme_Object* umemLoad(int argc, Scheme_Object* argv[]){
#endif
	SCM2UINT(argv[0], address);
	read_word_in_memory();
	return UINT2SCM(*(unsigned int*)rdword);
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

static Scheme_Env* env;

static int setup_mzscheme(Scheme_Env* e, int argc, char* argv[]){
	int i;
    mz_jmp_buf * volatile save, fresh;
    
    /* Declare embedded modules in "required.c": */
    declare_modules(e);
    
    scheme_namespace_require(scheme_intern_symbol("mzscheme"));
    
    // Create a Scheme function for each C function that needs to be called
    SCM_DEFUN(e, decodeNInterpret, "decode&interpret", 1, 1);
    SCM_DEFUN(e, regLoad, "reg", 1, 1);
    SCM_DEFUN(e, uregLoad, "ureg", 1, 1);
    SCM_DEFUN(e, reg64Load, "reg64", 1, 1);
    SCM_DEFUN(e, ureg64Load, "ureg64", 1, 1);
    SCM_DEFUN(e, regStore, "r=", 1, 1);
    SCM_DEFUN(e, uregStore, "ur=", 1, 1);
    SCM_DEFUN(e, reg64Store, "r64=", 1, 1);
    SCM_DEFUN(e, ureg64Store, "ur64=", 1, 1);
    SCM_DEFUN(e, umemLoad, "umemw", 1, 1);
	
	save = scheme_current_thread->error_buf;
    scheme_current_thread->error_buf = &fresh;
    
	if(scheme_setjmp(scheme_error_buf)){
		// This block will only be executed after a longjmp (an error in scheme code)
		scheme_current_thread->error_buf = save;
		env = NULL;
		return -1; /* There was an error */
	
	} else {
		// This block of code is executed immediately after setjmp
		// TODO: eval all the scheme code strings
		//scheme_eval_string("(hello-world)", e);
		scheme_current_thread->error_buf = save;
	}
	
	env = e;
	return 0;
}
#endif // MZ_SCHEME

int init_scheme_dynarec(void){
#ifdef MZ_SCHEME
	return scheme_main_setup(1, setup_mzscheme, 0, NULL);
#endif // MZ_SCHEME
}

unsigned int dynarec(unsigned int addr){
	char call[64];
	sprintf(call, "(dynarec #x%08x)", addr);
	unsigned int ret;
	SCM2UINT( scheme_eval_string(call, env), ret );
	return ret;
}
