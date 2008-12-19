#ifndef ASSEMBLE_H
#define ASSEMBLE_H

typedef struct _reg_cache_struct
{
	int need_map;
	void *needed_register[8];
	unsigned char jump_wrapper[62];
	int need_cop1_check;
} reg_cache_struct;

#endif // ASSEMBLE_H

