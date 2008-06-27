#ifndef FFI_H
#define FFI_H

#ifdef MZ_SCHEME
#define INT2SCM(i) \
	scheme_make_integer_value(i)
#define UINT2SCM(i) \
	scheme_make_integer_value_from_unsigned(i)
#define LONG2SCM(l) \
	scheme_make_integer_value_from_long_long(l)
#define ULONG2SCM(l) \
	scheme_make_integer_value_from_unsigned_long_long(l)

#define SCM2INT(s,i) \
	scheme_get_int_val(s, &i)
#define SCM2UINT(s,i) \
	scheme_get_unsigned_int_val(s, &i)
#define SCM2LONG(s,l) \
	scheme_get_long_long_val(s, &l)
#define SCM2ULONG(s,l) \
	scheme_get_unsigned_long_long_val(s, &l)

#define SCM_DEFUN(env,f,name,min,max) \
	scheme_add_global_constant( name, \
		scheme_make_prim_w_arity(f, name, min, max), env)

#endif // MZ_SCHEME

#endif FFI_H
