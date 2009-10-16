#include "2xSAI.h"


template <typename T>
struct _PixelIterator : PixelIterator {
private:
	T* ptr;
public:
	_PixelIterator(void* p) : ptr((T*)p) { }
	void operator ++ (){ ++ptr; }
	void operator += (int d){ ptr += d; }
	u32 operator [] (int offset){ return ptr[offset]; }
	void set(int offset, u32 value){ ptr[offset] = value; }
};

PixelIterator* Interpolator4444::iterator(void* p){
	return new _PixelIterator<u16>(p);
}
PixelIterator* Interpolator5551::iterator(void* p){
	return new _PixelIterator<u16>(p);
}
PixelIterator* Interpolator8888::iterator(void* p){
	return new _PixelIterator<u32>(p);
}

static inline s16 GetResult1( u32 A, u32 B, u32 C, u32 D, u32 E )
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

static inline s16 GetResult2( u32 A, u32 B, u32 C, u32 D, u32 E ) 
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

static inline s16 GetResult( u32 A, u32 B, u32 C, u32 D )
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


u32 Interpolator::interpolate(u32 A, u32 B){
	if(A != B)
		return (getHigh1(A) >> 1) +
		       (getHigh1(B) >> 1) |
		       getLow1(A & B);
	else
		return A;
}

u32 Interpolator::interpolate(u32 A, u32 B, u32 C, u32 D){
	u32 x = (getHigh2(A) >> 2) +
	        (getHigh2(B) >> 2) +
	        (getHigh2(C) >> 2) +
	        (getHigh2(D) >> 2);
	u32 y = getLow2((getLow2(A) +
	                 getLow2(B) +
	                 getLow2(C) +
	                 getLow2(D)) >> 2);
	u32 z = getZ(A, B, C, D);
	
	return x | y | z;
}


void _2xSaI( void *srcPtr, void *dstPtr,
             u16 width, u16 height, s32 clampS, s32 clampT,
             Interpolator* interpolator )
{
	u16 destWidth = width << 1;
	//u16 destHeight = height << 1;
	
	PixelIterator* srcIter = interpolator->iterator(srcPtr);
	PixelIterator* dstIter = interpolator->iterator(dstPtr);

	u32 colorA, colorB, colorC, colorD,
	    colorE, colorF, colorG, colorH,
	    colorI, colorJ, colorK, colorL,
	    colorM, colorN, colorO, colorP;
	u32 product, product1, product2;

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
            colorI = (*srcIter)[col0 + row0];
            colorE = (*srcIter)[col1 + row0];
            colorF = (*srcIter)[col2 + row0];
            colorJ = (*srcIter)[col3 + row0];

            colorG = (*srcIter)[col0 + row1];
            colorA = (*srcIter)[col1 + row1];
            colorB = (*srcIter)[col2 + row1];
            colorK = (*srcIter)[col3 + row1];

            colorH = (*srcIter)[col0 + row2];
            colorC = (*srcIter)[col1 + row2];
            colorD = (*srcIter)[col2 + row2];
            colorL = (*srcIter)[col3 + row2];

            colorM = (*srcIter)[col0 + row3];
            colorN = (*srcIter)[col1 + row3];
            colorO = (*srcIter)[col2 + row3];
            colorP = (*srcIter)[col3 + row3];

            if ((colorA == colorD) && (colorB != colorC))
            {
                if ( ((colorA == colorE) && (colorB == colorL)) ||
                    ((colorA == colorC) && (colorA == colorF) && (colorB != colorE) && (colorB == colorJ)) )
                    product = colorA;
                else
                    product = interpolator->interpolate(colorA, colorB);

                if (((colorA == colorG) && (colorC == colorO)) ||
                    ((colorA == colorB) && (colorA == colorH) && (colorG != colorC) && (colorC == colorM)) )
                    product1 = colorA;
                else
                    product1 = interpolator->interpolate(colorA, colorC);

                product2 = colorA;
            }
            else if ((colorB == colorC) && (colorA != colorD))
            {
                if (((colorB == colorF) && (colorA == colorH)) ||
                    ((colorB == colorE) && (colorB == colorD) && (colorA != colorF) && (colorA == colorI)) )
                    product = colorB;
                else
                    product = interpolator->interpolate(colorA, colorB);
 
                if (((colorC == colorH) && (colorA == colorF)) ||
                    ((colorC == colorG) && (colorC == colorD) && (colorA != colorH) && (colorA == colorI)) )
                    product1 = colorC;
                else
                    product1 = interpolator->interpolate(colorA, colorC);
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
                    product1 = interpolator->interpolate(colorA, colorC);
                    product = interpolator->interpolate(colorA, colorB);

                    r += GetResult1 (colorA, colorB, colorG, colorE, colorI);
                    r += GetResult2 (colorB, colorA, colorK, colorF, colorJ);
                    r += GetResult2 (colorB, colorA, colorH, colorN, colorM);
                    r += GetResult1 (colorA, colorB, colorL, colorO, colorP);

                    if (r > 0)
                        product2 = colorA;
                    else if (r < 0)
                        product2 = colorB;
                    else
                        product2 = interpolator->interpolate(colorA, colorB, colorC, colorD);
                }
            }
            else
            {
                product2 = interpolator->interpolate(colorA, colorB, colorC, colorD);

                if ((colorA == colorC) && (colorA == colorF) && (colorB != colorE) && (colorB == colorJ))
                    product = colorA;
                else if ((colorB == colorE) && (colorB == colorD) && (colorA != colorF) && (colorA == colorI))
                    product = colorB;
                else
                    product = interpolator->interpolate(colorA, colorB);

                if ((colorA == colorB) && (colorA == colorH) && (colorG != colorC) && (colorC == colorM))
                    product1 = colorA;
                else if ((colorC == colorG) && (colorC == colorD) && (colorA != colorH) && (colorA == colorI))
                    product1 = colorC;
                else
                    product1 = interpolator->interpolate(colorA, colorC);
            }

			dstIter->set(0, colorA);
			dstIter->set(1, product);
			dstIter->set(destWidth, product1);
			dstIter->set(destWidth + 1, product2);

			++(*srcIter);
			(*dstIter) += 2;
        }
		(*dstIter) += destWidth;
	}
	
	delete srcIter;
	delete dstIter;
}

