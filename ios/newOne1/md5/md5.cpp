#if 0
// Free for all implementation of the MD5 hash algorithm

/*
 **********************************************************************
 ** MD5.cpp                                                          **
 **                                                                  **
 ** - Style modified by Tony Ray, January 2001                       **
 **   Added support for randomizing initialization constants         **
 ** - Style modified by Dominik Reichl, April 2003                   **
 **   Optimized code                                                 **
 **                                                                  **
 **********************************************************************
 */

/*
 **********************************************************************
 ** MD5.c                                                            **
 ** RSA Data Security, Inc. MD5 Message Digest Algorithm             **
 ** Created: 2/17/90 RLR                                             **
 ** Revised: 1/91 SRD,AJ,BSK,JT Reference C Version                  **
 **********************************************************************
 */

/*
 **********************************************************************
 ** Copyright (C) 1990, RSA Data Security, Inc. All rights reserved. **
 **                                                                  **
 ** License to copy and use this software is granted provided that   **
 ** it is identified as the "RSA Data Security, Inc. MD5 Message     **
 ** Digest Algorithm" in all material mentioning or referencing this **
 ** software or this function.                                       **
 **                                                                  **
 ** License is also granted to make and use derivative works         **
 ** provided that such works are identified as "derived from the RSA **
 ** Data Security, Inc. MD5 Message Digest Algorithm" in all         **
 ** material mentioning or referencing the derived work.             **
 **                                                                  **
 ** RSA Data Security, Inc. makes no representations concerning      **
 ** either the merchantability of this software or the suitability   **
 ** of this software for any particular purpose.  It is provided "as **
 ** is" without express or implied warranty of any kind.             **
 **                                                                  **
 ** These notices must be retained in any copies of any part of this **
 ** documentation and/or software.                                   **
 **********************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
//#include "stdafx.h"
#include "md5.h"

/* Padding */
static unsigned char MD5_PADDING[64] = {
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* MD5_F, MD5_G and MD5_H are basic MD5 functions: selection, majority, parity */
#define MD5_F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define MD5_G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define MD5_H(x, y, z) ((x) ^ (y) ^ (z))
#define MD5_I(x, y, z) ((y) ^ ((x) | (~z)))

/* ROTATE_LEFT rotates x left n bits */
#ifndef ROTATE_LEFT
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))
#endif

/* MD5_FF, MD5_GG, MD5_HH, and MD5_II transformations for rounds 1, 2, 3, and 4 */
/* Rotation is separate from addition to prevent recomputation */
#define MD5_FF(a, b, c, d, x, s, ac) {(a) += MD5_F ((b), (c), (d)) + (x) + (UINT4)(ac); (a) = ROTATE_LEFT ((a), (s)); (a) += (b); }
#define MD5_GG(a, b, c, d, x, s, ac) {(a) += MD5_G ((b), (c), (d)) + (x) + (UINT4)(ac); (a) = ROTATE_LEFT ((a), (s)); (a) += (b); }
#define MD5_HH(a, b, c, d, x, s, ac) {(a) += MD5_H ((b), (c), (d)) + (x) + (UINT4)(ac); (a) = ROTATE_LEFT ((a), (s)); (a) += (b); }
#define MD5_II(a, b, c, d, x, s, ac) {(a) += MD5_I ((b), (c), (d)) + (x) + (UINT4)(ac); (a) = ROTATE_LEFT ((a), (s)); (a) += (b); }

/* Constants for transformation */
#define MD5_S11 7  /* Round 1 */
#define MD5_S12 12
#define MD5_S13 17
#define MD5_S14 22
#define MD5_S21 5  /* Round 2 */
#define MD5_S22 9
#define MD5_S23 14
#define MD5_S24 20
#define MD5_S31 4  /* Round 3 */
#define MD5_S32 11
#define MD5_S33 16
#define MD5_S34 23
#define MD5_S41 6  /* Round 4 */
#define MD5_S42 10
#define MD5_S43 15
#define MD5_S44 21

/* Basic MD5 step. MD5_Transform buf based on in */
static void MD5_Transform (UINT4 *buf, UINT4 *in)
{
	UINT4 a = buf[0], b = buf[1], c = buf[2], d = buf[3];

	/* Round 1 */
	MD5_FF ( a, b, c, d, in[ 0], MD5_S11, (UINT4) 3614090360u); /* 1 */
	MD5_FF ( d, a, b, c, in[ 1], MD5_S12, (UINT4) 3905402710u); /* 2 */
	MD5_FF ( c, d, a, b, in[ 2], MD5_S13, (UINT4)  606105819u); /* 3 */
	MD5_FF ( b, c, d, a, in[ 3], MD5_S14, (UINT4) 3250441966u); /* 4 */
	MD5_FF ( a, b, c, d, in[ 4], MD5_S11, (UINT4) 4118548399u); /* 5 */
	MD5_FF ( d, a, b, c, in[ 5], MD5_S12, (UINT4) 1200080426u); /* 6 */
	MD5_FF ( c, d, a, b, in[ 6], MD5_S13, (UINT4) 2821735955u); /* 7 */
	MD5_FF ( b, c, d, a, in[ 7], MD5_S14, (UINT4) 4249261313u); /* 8 */
	MD5_FF ( a, b, c, d, in[ 8], MD5_S11, (UINT4) 1770035416u); /* 9 */
	MD5_FF ( d, a, b, c, in[ 9], MD5_S12, (UINT4) 2336552879u); /* 10 */
	MD5_FF ( c, d, a, b, in[10], MD5_S13, (UINT4) 4294925233u); /* 11 */
	MD5_FF ( b, c, d, a, in[11], MD5_S14, (UINT4) 2304563134u); /* 12 */
	MD5_FF ( a, b, c, d, in[12], MD5_S11, (UINT4) 1804603682u); /* 13 */
	MD5_FF ( d, a, b, c, in[13], MD5_S12, (UINT4) 4254626195u); /* 14 */
	MD5_FF ( c, d, a, b, in[14], MD5_S13, (UINT4) 2792965006u); /* 15 */
	MD5_FF ( b, c, d, a, in[15], MD5_S14, (UINT4) 1236535329u); /* 16 */

	/* Round 2 */
	MD5_GG ( a, b, c, d, in[ 1], MD5_S21, (UINT4) 4129170786u); /* 17 */
	MD5_GG ( d, a, b, c, in[ 6], MD5_S22, (UINT4) 3225465664u); /* 18 */
	MD5_GG ( c, d, a, b, in[11], MD5_S23, (UINT4)  643717713u); /* 19 */
	MD5_GG ( b, c, d, a, in[ 0], MD5_S24, (UINT4) 3921069994u); /* 20 */
	MD5_GG ( a, b, c, d, in[ 5], MD5_S21, (UINT4) 3593408605u); /* 21 */
	MD5_GG ( d, a, b, c, in[10], MD5_S22, (UINT4)   38016083u); /* 22 */
	MD5_GG ( c, d, a, b, in[15], MD5_S23, (UINT4) 3634488961u); /* 23 */
	MD5_GG ( b, c, d, a, in[ 4], MD5_S24, (UINT4) 3889429448u); /* 24 */
	MD5_GG ( a, b, c, d, in[ 9], MD5_S21, (UINT4)  568446438u); /* 25 */
	MD5_GG ( d, a, b, c, in[14], MD5_S22, (UINT4) 3275163606u); /* 26 */
	MD5_GG ( c, d, a, b, in[ 3], MD5_S23, (UINT4) 4107603335u); /* 27 */
	MD5_GG ( b, c, d, a, in[ 8], MD5_S24, (UINT4) 1163531501u); /* 28 */
	MD5_GG ( a, b, c, d, in[13], MD5_S21, (UINT4) 2850285829u); /* 29 */
	MD5_GG ( d, a, b, c, in[ 2], MD5_S22, (UINT4) 4243563512u); /* 30 */
	MD5_GG ( c, d, a, b, in[ 7], MD5_S23, (UINT4) 1735328473u); /* 31 */
	MD5_GG ( b, c, d, a, in[12], MD5_S24, (UINT4) 2368359562u); /* 32 */

	/* Round 3 */
	MD5_HH ( a, b, c, d, in[ 5], MD5_S31, (UINT4) 4294588738u); /* 33 */
	MD5_HH ( d, a, b, c, in[ 8], MD5_S32, (UINT4) 2272392833u); /* 34 */
	MD5_HH ( c, d, a, b, in[11], MD5_S33, (UINT4) 1839030562u); /* 35 */
	MD5_HH ( b, c, d, a, in[14], MD5_S34, (UINT4) 4259657740u); /* 36 */
	MD5_HH ( a, b, c, d, in[ 1], MD5_S31, (UINT4) 2763975236u); /* 37 */
	MD5_HH ( d, a, b, c, in[ 4], MD5_S32, (UINT4) 1272893353u); /* 38 */
	MD5_HH ( c, d, a, b, in[ 7], MD5_S33, (UINT4) 4139469664u); /* 39 */
	MD5_HH ( b, c, d, a, in[10], MD5_S34, (UINT4) 3200236656u); /* 40 */
	MD5_HH ( a, b, c, d, in[13], MD5_S31, (UINT4)  681279174u); /* 41 */
	MD5_HH ( d, a, b, c, in[ 0], MD5_S32, (UINT4) 3936430074u); /* 42 */
	MD5_HH ( c, d, a, b, in[ 3], MD5_S33, (UINT4) 3572445317u); /* 43 */
	MD5_HH ( b, c, d, a, in[ 6], MD5_S34, (UINT4)   76029189u); /* 44 */
	MD5_HH ( a, b, c, d, in[ 9], MD5_S31, (UINT4) 3654602809u); /* 45 */
	MD5_HH ( d, a, b, c, in[12], MD5_S32, (UINT4) 3873151461u); /* 46 */
	MD5_HH ( c, d, a, b, in[15], MD5_S33, (UINT4)  530742520u); /* 47 */
	MD5_HH ( b, c, d, a, in[ 2], MD5_S34, (UINT4) 3299628645u); /* 48 */

	/* Round 4 */
	MD5_II ( a, b, c, d, in[ 0], MD5_S41, (UINT4) 4096336452u); /* 49 */
	MD5_II ( d, a, b, c, in[ 7], MD5_S42, (UINT4) 1126891415u); /* 50 */
	MD5_II ( c, d, a, b, in[14], MD5_S43, (UINT4) 2878612391u); /* 51 */
	MD5_II ( b, c, d, a, in[ 5], MD5_S44, (UINT4) 4237533241u); /* 52 */
	MD5_II ( a, b, c, d, in[12], MD5_S41, (UINT4) 1700485571u); /* 53 */
	MD5_II ( d, a, b, c, in[ 3], MD5_S42, (UINT4) 2399980690u); /* 54 */
	MD5_II ( c, d, a, b, in[10], MD5_S43, (UINT4) 4293915773u); /* 55 */
	MD5_II ( b, c, d, a, in[ 1], MD5_S44, (UINT4) 2240044497u); /* 56 */
	MD5_II ( a, b, c, d, in[ 8], MD5_S41, (UINT4) 1873313359u); /* 57 */
	MD5_II ( d, a, b, c, in[15], MD5_S42, (UINT4) 4264355552u); /* 58 */
	MD5_II ( c, d, a, b, in[ 6], MD5_S43, (UINT4) 2734768916u); /* 59 */
	MD5_II ( b, c, d, a, in[13], MD5_S44, (UINT4) 1309151649u); /* 60 */
	MD5_II ( a, b, c, d, in[ 4], MD5_S41, (UINT4) 4149444226u); /* 61 */
	MD5_II ( d, a, b, c, in[11], MD5_S42, (UINT4) 3174756917u); /* 62 */
	MD5_II ( c, d, a, b, in[ 2], MD5_S43, (UINT4)  718787259u); /* 63 */
	MD5_II ( b, c, d, a, in[ 9], MD5_S44, (UINT4) 3951481745u); /* 64 */

	buf[0] += a;
	buf[1] += b;
	buf[2] += c;
	buf[3] += d;
}

// Set pseudoRandomNumber to zero for RFC MD5 implementation
void MD5Init (MD5_CTX *mdContext, unsigned long pseudoRandomNumber)
{
	mdContext->i[0] = mdContext->i[1] = (UINT4)0;

	/* Load magic initialization constants */
	mdContext->buf[0] = (UINT4)0x67452301 + (pseudoRandomNumber * 11);
	mdContext->buf[1] = (UINT4)0xefcdab89 + (pseudoRandomNumber * 71);
	mdContext->buf[2] = (UINT4)0x98badcfe + (pseudoRandomNumber * 37);
	mdContext->buf[3] = (UINT4)0x10325476 + (pseudoRandomNumber * 97);
}

void MD5Update (MD5_CTX *mdContext, unsigned char *inBuf, unsigned int inLen)
{
	UINT4 in[16];
	int mdi = 0;
	unsigned int i = 0, ii = 0;

	/* Compute number of bytes mod 64 */
	mdi = (int)((mdContext->i[0] >> 3) & 0x3F);

	/* Update number of bits */
	if ((mdContext->i[0] + ((UINT4)inLen << 3)) < mdContext->i[0])
		mdContext->i[1]++;
	mdContext->i[0] += ((UINT4)inLen << 3);
	mdContext->i[1] += ((UINT4)inLen >> 29);

	while (inLen--)
	{
		/* Add new character to buffer, increment mdi */
		mdContext->in[mdi++] = *inBuf++;

		/* Transform if necessary */
		if (mdi == 0x40)
		{
			for (i = 0, ii = 0; i < 16; i++, ii += 4)
				in[i] = (((UINT4)mdContext->in[ii+3]) << 24) |
					(((UINT4)mdContext->in[ii+2]) << 16) |
					(((UINT4)mdContext->in[ii+1]) << 8) |
					((UINT4)mdContext->in[ii]);

			MD5_Transform (mdContext->buf, in);
			mdi = 0;
		}
	}
}

void MD5Final (MD5_CTX *mdContext)
{
	UINT4 in[16];
	int mdi = 0;
	unsigned int i = 0, ii = 0, padLen = 0;

	/* Save number of bits */
	in[14] = mdContext->i[0];
	in[15] = mdContext->i[1];

	/* Compute number of bytes mod 64 */
	mdi = (int)((mdContext->i[0] >> 3) & 0x3F);

	/* Pad out to 56 mod 64 */
	padLen = (mdi < 56) ? (56 - mdi) : (120 - mdi);
	MD5Update (mdContext, MD5_PADDING, padLen);

	/* Append length in bits and transform */
	for (i = 0, ii = 0; i < 14; i++, ii += 4)
		in[i] = (((UINT4)mdContext->in[ii+3]) << 24) |
			(((UINT4)mdContext->in[ii+2]) << 16) |
			(((UINT4)mdContext->in[ii+1]) <<  8) |
			((UINT4)mdContext->in[ii]);
	MD5_Transform (mdContext->buf, in);

	/* Store buffer in digest */
	for (i = 0, ii = 0; i < 4; i++, ii += 4)
	{
		mdContext->digest[ii]   = (unsigned char)( mdContext->buf[i]        & 0xFF);
		mdContext->digest[ii+1] = (unsigned char)((mdContext->buf[i] >>  8) & 0xFF);
		mdContext->digest[ii+2] = (unsigned char)((mdContext->buf[i] >> 16) & 0xFF);
		mdContext->digest[ii+3] = (unsigned char)((mdContext->buf[i] >> 24) & 0xFF);
	}
}
//
// md5file
//
int md5file ( char *fn , unsigned long seed , MD5_CTX *mdContext )
{

	unsigned long size = 0;
	FILE *f = fopen ( fn , "rb" ) ;
	if ( f == NULL ) return 0 ;		

	MD5Init ( mdContext , seed ) ;

	char buf[2048] ;
	unsigned int trb = 0 ;
	for (;;) {
		int rb = fread ( buf , 1 , 2048 , f ) ;
		if ( size > 0 && rb + trb > size ) rb = size - trb ;
		trb += rb ;
		MD5Update ( mdContext , (unsigned char *) buf , rb ) ;
		if ( rb < 2048 || ( size > 0 && trb >= size ) ) break ;
	}
	fclose ( f ) ;
	MD5Final ( mdContext ) ;

	return trb ;
}

#else

#include "MD5.h"

/* system implementation headers */
#include <stdio.h>
#include <string.h>


// Constants for MD5Transform routine.
#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

///////////////////////////////////////////////

// F, G, H and I are basic MD5 functions.
inline MD5::uint4 MD5::F(uint4 x, uint4 y, uint4 z) {
    return x&y | ~x&z;
}

inline MD5::uint4 MD5::G(uint4 x, uint4 y, uint4 z) {
    return x&z | y&~z;
}

inline MD5::uint4 MD5::H(uint4 x, uint4 y, uint4 z) {
    return x^y^z;
}

inline MD5::uint4 MD5::I(uint4 x, uint4 y, uint4 z) {
    return y ^ (x | ~z);
}

// rotate_left rotates x left n bits.
inline MD5::uint4 MD5::rotate_left(uint4 x, int n) {
    return (x << n) | (x >> (32 - n));
}

// FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
// Rotation is separate from addition to prevent recomputation.
inline void MD5::FF(uint4 &a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac) {
    a = rotate_left(a + F(b, c, d) + x + ac, s) + b;
}

inline void MD5::GG(uint4 &a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac) {
    a = rotate_left(a + G(b, c, d) + x + ac, s) + b;
}

inline void MD5::HH(uint4 &a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac) {
    a = rotate_left(a + H(b, c, d) + x + ac, s) + b;
}

inline void MD5::II(uint4 &a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac) {
    a = rotate_left(a + I(b, c, d) + x + ac, s) + b;
}

//////////////////////////////////////////////

// default ctor, just initailize
MD5::MD5()
{
    init();
}

//////////////////////////////////////////////

// nifty shortcut ctor, compute MD5 for string and finalize it right away
MD5::MD5(const std::string &text)
{
    init();
    update(text.c_str(), text.length());
    finalize();
}

//////////////////////////////

void MD5::init()
{
    finalized = false;

    count[0] = 0;
    count[1] = 0;

    // load magic initialization constants.
    state[0] = 0x67452301;
    state[1] = 0xefcdab89;
    state[2] = 0x98badcfe;
    state[3] = 0x10325476;
}

//////////////////////////////

// decodes input (unsigned char) into output (uint4). Assumes len is a multiple of 4.
void MD5::decode(uint4 output[], const uint1 input[], size_type len)
{
    for (unsigned int i = 0, j = 0; j < len; i++, j += 4)
        output[i] = ((uint4)input[j]) | (((uint4)input[j + 1]) << 8) |
                    (((uint4)input[j + 2]) << 16) | (((uint4)input[j + 3]) << 24);
}

//////////////////////////////

// encodes input (uint4) into output (unsigned char). Assumes len is
// a multiple of 4.
void MD5::encode(uint1 output[], const uint4 input[], size_type len)
{
    for (size_type i = 0, j = 0; j < len; i++, j += 4) {
        output[j] = input[i] & 0xff;
        output[j + 1] = (input[i] >> 8) & 0xff;
        output[j + 2] = (input[i] >> 16) & 0xff;
        output[j + 3] = (input[i] >> 24) & 0xff;
    }
}

//////////////////////////////

// apply MD5 algo on a block
void MD5::transform(const uint1 block[blocksize])
{
    uint4 a = state[0], b = state[1], c = state[2], d = state[3], x[16];
    decode(x, block, blocksize);

    /* Round 1 */
    FF(a, b, c, d, x[0], S11, 0xd76aa478); /* 1 */
    FF(d, a, b, c, x[1], S12, 0xe8c7b756); /* 2 */
    FF(c, d, a, b, x[2], S13, 0x242070db); /* 3 */
    FF(b, c, d, a, x[3], S14, 0xc1bdceee); /* 4 */
    FF(a, b, c, d, x[4], S11, 0xf57c0faf); /* 5 */
    FF(d, a, b, c, x[5], S12, 0x4787c62a); /* 6 */
    FF(c, d, a, b, x[6], S13, 0xa8304613); /* 7 */
    FF(b, c, d, a, x[7], S14, 0xfd469501); /* 8 */
    FF(a, b, c, d, x[8], S11, 0x698098d8); /* 9 */
    FF(d, a, b, c, x[9], S12, 0x8b44f7af); /* 10 */
    FF(c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
    FF(b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
    FF(a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
    FF(d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
    FF(c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
    FF(b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

    /* Round 2 */
    GG(a, b, c, d, x[1], S21, 0xf61e2562); /* 17 */
    GG(d, a, b, c, x[6], S22, 0xc040b340); /* 18 */
    GG(c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
    GG(b, c, d, a, x[0], S24, 0xe9b6c7aa); /* 20 */
    GG(a, b, c, d, x[5], S21, 0xd62f105d); /* 21 */
    GG(d, a, b, c, x[10], S22, 0x2441453); /* 22 */
    GG(c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
    GG(b, c, d, a, x[4], S24, 0xe7d3fbc8); /* 24 */
    GG(a, b, c, d, x[9], S21, 0x21e1cde6); /* 25 */
    GG(d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
    GG(c, d, a, b, x[3], S23, 0xf4d50d87); /* 27 */
    GG(b, c, d, a, x[8], S24, 0x455a14ed); /* 28 */
    GG(a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
    GG(d, a, b, c, x[2], S22, 0xfcefa3f8); /* 30 */
    GG(c, d, a, b, x[7], S23, 0x676f02d9); /* 31 */
    GG(b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

    /* Round 3 */
    HH(a, b, c, d, x[5], S31, 0xfffa3942); /* 33 */
    HH(d, a, b, c, x[8], S32, 0x8771f681); /* 34 */
    HH(c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
    HH(b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
    HH(a, b, c, d, x[1], S31, 0xa4beea44); /* 37 */
    HH(d, a, b, c, x[4], S32, 0x4bdecfa9); /* 38 */
    HH(c, d, a, b, x[7], S33, 0xf6bb4b60); /* 39 */
    HH(b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
    HH(a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
    HH(d, a, b, c, x[0], S32, 0xeaa127fa); /* 42 */
    HH(c, d, a, b, x[3], S33, 0xd4ef3085); /* 43 */
    HH(b, c, d, a, x[6], S34, 0x4881d05); /* 44 */
    HH(a, b, c, d, x[9], S31, 0xd9d4d039); /* 45 */
    HH(d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
    HH(c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
    HH(b, c, d, a, x[2], S34, 0xc4ac5665); /* 48 */

    /* Round 4 */
    II(a, b, c, d, x[0], S41, 0xf4292244); /* 49 */
    II(d, a, b, c, x[7], S42, 0x432aff97); /* 50 */
    II(c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
    II(b, c, d, a, x[5], S44, 0xfc93a039); /* 52 */
    II(a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
    II(d, a, b, c, x[3], S42, 0x8f0ccc92); /* 54 */
    II(c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
    II(b, c, d, a, x[1], S44, 0x85845dd1); /* 56 */
    II(a, b, c, d, x[8], S41, 0x6fa87e4f); /* 57 */
    II(d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
    II(c, d, a, b, x[6], S43, 0xa3014314); /* 59 */
    II(b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
    II(a, b, c, d, x[4], S41, 0xf7537e82); /* 61 */
    II(d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
    II(c, d, a, b, x[2], S43, 0x2ad7d2bb); /* 63 */
    II(b, c, d, a, x[9], S44, 0xeb86d391); /* 64 */

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;

    // Zeroize sensitive information.
    memset(x, 0, sizeof x);
}

//////////////////////////////

// MD5 block update operation. Continues an MD5 message-digest
// operation, processing another message block
void MD5::update(const unsigned char input[], size_type length)
{
    // compute number of bytes mod 64
    size_type index = count[0] / 8 % blocksize;

    // Update number of bits
    if ((count[0] += (length << 3)) < (length << 3))
        count[1]++;
    count[1] += (length >> 29);

    // number of bytes we need to fill in buffer
    size_type firstpart = 64 - index;

    size_type i;

    // transform as many times as possible.
    if (length >= firstpart)
    {
        // fill buffer first, transform
        memcpy(&buffer[index], input, firstpart);
        transform(buffer);

        // transform chunks of blocksize (64 bytes)
        for (i = firstpart; i + blocksize <= length; i += blocksize)
            transform(&input[i]);

        index = 0;
    }
    else
        i = 0;

    // buffer remaining input
    memcpy(&buffer[index], &input[i], length - i);
}

//////////////////////////////

// for convenience provide a verson with signed char
void MD5::update(const char input[], size_type length)
{
    update((const unsigned char*)input, length);
}

//////////////////////////////

// MD5 finalization. Ends an MD5 message-digest operation, writing the
// the message digest and zeroizing the context.
MD5& MD5::finalize()
{
    static unsigned char padding[64] = {
            0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    if (!finalized) {
        // Save number of bits
        unsigned char bits[8];
        encode(bits, count, 8);

        // pad out to 56 mod 64.
        size_type index = count[0] / 8 % 64;
        size_type padLen = (index < 56) ? (56 - index) : (120 - index);
        update(padding, padLen);

        // Append length (before padding)
        update(bits, 8);

        // Store state in digest
        encode(digest, state, 16);

        // Zeroize sensitive information.
        memset(buffer, 0, sizeof buffer);
        memset(count, 0, sizeof count);

        finalized = true;
    }

    return *this;
}

//////////////////////////////

// return hex representation of digest as string
std::string MD5::hexdigest() const
{
    if (!finalized)
        return "";

    char buf[33];
    for (int i = 0; i < 16; i++)
        sprintf(buf + i * 2, "%02x", digest[i]);
    buf[32] = 0;

    return std::string(buf);
}

//////////////////////////////

std::ostream& operator<<(std::ostream& out, MD5 md5)
{
    return out << md5.hexdigest();
}

//////////////////////////////

std::string md5(const std::string str)
{
    MD5 md5 = MD5(str);

    return md5.hexdigest();
}
#endif