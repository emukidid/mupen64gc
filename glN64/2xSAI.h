#ifndef _2XSAI_H
#define _2XSAI_H
#include "Types.h"


struct PixelIterator {
	virtual void operator ++ () = 0;
	virtual void operator += (int) = 0;
	virtual u32 operator [] (int) = 0;
	virtual void set(int offset, u32 value) = 0;
};


struct Interpolator {
	virtual u32 interpolate(u32, u32);
	virtual u32 interpolate(u32, u32, u32, u32);
	virtual PixelIterator* iterator(void*) = 0;
protected:
	virtual u32 getHigh1(u32 color) = 0;
	virtual u32 getLow1(u32 color) = 0;
	virtual u32 getHigh2(u32 color) = 0;
	virtual u32 getLow2(u32 color) = 0;
	virtual u32 getZ(u32, u32, u32, u32){ return 0; }
};

class Interpolator4444 : public Interpolator {
public:
	PixelIterator* iterator(void*);
protected:
	u32 getHigh1(u32 color){ return color & 0xEEEE; }
	u32 getLow1(u32 color){ return color & 0x1111; }
	u32 getHigh2(u32 color){ return color & 0xCCCC; }
	u32 getLow2(u32 color){ return color & 0x3333; }
};

class Interpolator5551 : public Interpolator {
public:
	PixelIterator* iterator(void*);
protected:
	u32 getHigh1(u32 color){ return color & 0xF7BC; }
	u32 getLow1(u32 color){ return color & 0x0843; }
	u32 getHigh2(u32 color){ return color & 0xE738; }
	u32 getLow2(u32 color){ return color & 0x18C6; }
	u32 getZ(u32 A, u32 B, u32 C, u32 D){
		return ((A & 1) + (B & 1) + (C & 1) + (D & 1)) > 2 ? 1 : 0;
	}
};

class Interpolator8888 : public Interpolator {
public:
	PixelIterator* iterator(void*);
protected:
	u32 getHigh1(u32 color){ return color & 0xFEFEFEFE; }
	u32 getLow1(u32 color){ return color & 0x01010101; }
	u32 getHigh2(u32 color){ return color & 0xFCFCFCFC; }
	u32 getLow2(u32 color){ return color & 0x03030303; }
};

void _2xSaI( void *srcPtr, void *destPtr,
             u16 width, u16 height, s32 clampS, s32 clampT,
             Interpolator* interpolator );

#endif

