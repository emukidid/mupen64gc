#ifndef _2XSAI_H
#define _2XSAI_H
#include "Types.h"

#pragma pack(1)

struct Color4444 {
	union {
		u16 color;
		struct {
			unsigned r : 4;
			unsigned g : 4;
			unsigned b : 4;
			unsigned a : 4;
		} rgba;
	};
	static u16 mask;
	static u16 qmask;
	Color4444() : color(0) { }
	Color4444(u16 color) : color(color) { }
	Color4444(const Color4444& o) : color(o.color) { }
	static u16 get_z(const Color4444&, const Color4444&, const Color4444&, const Color4444&){
		return 0;
	}
};
u16 Color4444::mask = 0x1111;
u16 Color4444::qmask = 0x3333;
bool operator == (const Color4444& a, const Color4444& b){ return a.color == b.color; }
bool operator != (const Color4444& a, const Color4444& b){ return a.color != b.color; }

struct Color5551 {
	union {
		u16 color;
		struct {
			unsigned r : 5;
			unsigned g : 5;
			unsigned b : 5;
			unsigned a : 1;
		} rgba;
	};
	static u16 mask;
	static u16 qmask;
	Color5551() : color(0) { }
	Color5551(u16 color) : color(color) { }
	Color5551(const Color5551& o) : color(o.color) { }
	static u16 get_z(const Color5551& A, const Color5551& B, const Color5551& C, const Color5551& D){
		return ((A.color & 0x0001) +
		        (B.color & 0x0001) +
		        (C.color & 0x0001) +
		        (D.color & 0x0001)) > 2 ? 1 : 0;
	}
};
u16 Color5551::mask = 0x0843;
u16 Color5551::qmask = 0x18C6;
bool operator == (const Color5551& a, const Color5551& b){ return a.color == b.color; }
bool operator != (const Color5551& a, const Color5551& b){ return a.color != b.color; }

struct Color8888 {
	union {
		u32 color;
		struct {
			unsigned r : 8;
			unsigned g : 8;
			unsigned b : 8;
			unsigned a : 8;
		} rgba;
	};
	static u32 mask;
	static u32 qmask;
	Color8888() : color(0) { }
	Color8888(u32 color) : color(color) { }
	Color8888(const Color8888& o) : color(o.color) { }
	static u32 get_z(const Color8888&, const Color8888&, const Color8888&, const Color8888&){
		return 0;
	}
};
u32 Color8888::mask = 0x01010101;
u32 Color8888::qmask = 0x03030303;
bool operator == (const Color8888& a, const Color8888& b){ return a.color == b.color; }
bool operator != (const Color8888& a, const Color8888& b){ return a.color != b.color; }

#pragma pack()

template <typename T>
static inline s16 GetResult1( const T& A, const T& B, const T& C, const T& D, const T& E )
{
	s16 x = 0;
	s16 y = 0;
	s16 r = 0;

	if (A == C) x += 1; else if (B == C) y += 1;
	if (A == D) x += 1; else if (B == D) y += 1;
	if (x <= 1) r += 1; 
	if (y <= 1) r -= 1;

	return r;
}

template <typename T>
static inline s16 GetResult2( const T& A, const T& B, const T& C, const T& D, const T& E) 
{
	s16 x = 0; 
	s16 y = 0;
	s16 r = 0;

	if (A == C) x += 1; else if (B == C) y += 1;
	if (A == D) x += 1; else if (B == D) y += 1;
	if (x <= 1) r -= 1; 
	if (y <= 1) r += 1;

	return r;
}

template <typename T>
static inline s16 GetResult( const T& A, const T& B, const T& C, const T& D )
{
	s16 x = 0; 
	s16 y = 0;
	s16 r = 0;

	if (A == C) x += 1; else if (B == C) y += 1;
	if (A == D) x += 1; else if (B == D) y += 1;
	if (x <= 1) r += 1; 
	if (y <= 1) r -= 1;

	return r;
}


template <typename T>
static inline T interpolate(const T& A, const T& B){
	if(A.color != B.color)
		return T(((A.color & ~T::mask) >> 1) +
		         ((B.color & ~T::mask) >> 1) |
		         (A.color & B.color & T::mask));
	else
		return A;
}

template <typename T>
static inline T interpolate(const T& A, const T& B, const T& C, const T& D){
	T x =	T( ((A.color & ~T::qmask) >> 2) +
				 ((B.color & ~T::qmask) >> 2) +
				 ((C.color & ~T::qmask) >> 2) +
				 ((D.color & ~T::qmask) >> 2) );
	T y =	T( (((A.color & T::qmask) +
				  (B.color & T::qmask) +
				  (C.color & T::qmask) +
				  (D.color & T::qmask)) >> 2) & T::qmask );
	T z = 	T( T::get_z(A, B, C, D) );
	
	return T(x.color | y.color | z.color);
}


template <typename T>
void _2xSaI( T *srcPtr, T *destPtr, u16 width, u16 height, s32 clampS, s32 clampT )
{
	u16 destWidth = width << 1;
	//u16 destHeight = height << 1;

	T colorA, colorB, colorC, colorD,
	  colorE, colorF, colorG, colorH,
	  colorI, colorJ, colorK, colorL,
	  colorM, colorN, colorO, colorP;
	T product, product1, product2;

	s16 row0, row1, row2, row3;
	s16 col0, col1, col2, col3;

	for (u16 y = 0; y < height; y++)
	{
		if (y > 0)
			row0 = -width;
		else
			row0 = clampT ? 0 : (height - 1) * width;

		row1 = 0;

		if (y < height - 1)
		{
			row2 = width;

			if (y < height - 2) 
				row3 = width << 1;
			else
				row3 = clampT ? width : -y * width;
		}
		else
		{
			row2 = clampT ? 0 : -y * width;
			row3 = clampT ? 0 : (1 - y) * width;
		}

        for (u16 x = 0; x < width; x++)
        {
			if (x > 0)
				col0 = -1;
			else
				col0 = clampS ? 0 : width - 1;

			col1 = 0;

			if (x < width - 1)
			{
				col2 = 1;

				if (x < width - 2) 
					col3 = 2;
				else
					col3 = clampS ? 1 : -x;
			}
			else
			{
				col2 = clampS ? 0 : -x;
				col3 = clampS ? 0 : 1 - x;
			}

//---------------------------------------
// Map of the pixels:                    I|E F|J
//                                       G|A B|K
//                                       H|C D|L
//                                       M|N O|P
            colorI = *(srcPtr + col0 + row0);
            colorE = *(srcPtr + col1 + row0);
            colorF = *(srcPtr + col2 + row0);
            colorJ = *(srcPtr + col3 + row0);

            colorG = *(srcPtr + col0 + row1);
            colorA = *(srcPtr + col1 + row1);
            colorB = *(srcPtr + col2 + row1);
            colorK = *(srcPtr + col3 + row1);

            colorH = *(srcPtr + col0 + row2);
            colorC = *(srcPtr + col1 + row2);
            colorD = *(srcPtr + col2 + row2);
            colorL = *(srcPtr + col3 + row2);

            colorM = *(srcPtr + col0 + row3);
            colorN = *(srcPtr + col1 + row3);
            colorO = *(srcPtr + col2 + row3);
            colorP = *(srcPtr + col3 + row3);

            if ((colorA == colorD) && (colorB != colorC))
            {
                if ( ((colorA == colorE) && (colorB == colorL)) ||
                    ((colorA == colorC) && (colorA == colorF) && (colorB != colorE) && (colorB == colorJ)) )
                    product = colorA;
                else
                    product = interpolate(colorA, colorB);

                if (((colorA == colorG) && (colorC == colorO)) ||
                    ((colorA == colorB) && (colorA == colorH) && (colorG != colorC) && (colorC == colorM)) )
                    product1 = colorA;
                else
                    product1 = interpolate(colorA, colorC);

                product2 = colorA;
            }
            else if ((colorB == colorC) && (colorA != colorD))
            {
                if (((colorB == colorF) && (colorA == colorH)) ||
                    ((colorB == colorE) && (colorB == colorD) && (colorA != colorF) && (colorA == colorI)) )
                    product = colorB;
                else
                    product = interpolate(colorA, colorB);
 
                if (((colorC == colorH) && (colorA == colorF)) ||
                    ((colorC == colorG) && (colorC == colorD) && (colorA != colorH) && (colorA == colorI)) )
                    product1 = colorC;
                else
                    product1 = interpolate(colorA, colorC);
                product2 = colorB;
            }
            else if ((colorA == colorD) && (colorB == colorC))
            {
                if (colorA == colorB)
                {
                    product = colorA;
                    product1 = colorA;
                    product2 = colorA;
                }
                else
                {
                    s16 r = 0;
                    product1 = interpolate(colorA, colorC);
                    product = interpolate(colorA, colorB);

                    r += GetResult1 (colorA, colorB, colorG, colorE, colorI);
                    r += GetResult2 (colorB, colorA, colorK, colorF, colorJ);
                    r += GetResult2 (colorB, colorA, colorH, colorN, colorM);
                    r += GetResult1 (colorA, colorB, colorL, colorO, colorP);

                    if (r > 0)
                        product2 = colorA;
                    else if (r < 0)
                        product2 = colorB;
                    else
                        product2 = interpolate(colorA, colorB, colorC, colorD);
                }
            }
            else
            {
                product2 = interpolate(colorA, colorB, colorC, colorD);

                if ((colorA == colorC) && (colorA == colorF) && (colorB != colorE) && (colorB == colorJ))
                    product = colorA;
                else if ((colorB == colorE) && (colorB == colorD) && (colorA != colorF) && (colorA == colorI))
                    product = colorB;
                else
                    product = interpolate(colorA, colorB);

                if ((colorA == colorB) && (colorA == colorH) && (colorG != colorC) && (colorC == colorM))
                    product1 = colorA;
                else if ((colorC == colorG) && (colorC == colorD) && (colorA != colorH) && (colorA == colorI))
                    product1 = colorC;
                else
                    product1 = interpolate(colorA, colorC);
            }

			destPtr[0] = colorA;
			destPtr[1] = product;
			destPtr[destWidth] = product1;
			destPtr[destWidth + 1] = product2;

			srcPtr++;
			destPtr += 2;
        }
		destPtr += destWidth;
	}
}

#endif

