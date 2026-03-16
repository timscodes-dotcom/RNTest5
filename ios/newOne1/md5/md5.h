#if 0
/*
 **********************************************************************
 ** MD5.h                                                            **
 **                                                                  **
 ** - Style modified by Tony Ray, January 2001                       **
 **   Added support for randomizing initialization constants         **
 ** - Style modified by Dominik Reichl, September 2002               **
 **   Optimized code                                                 **
 **                                                                  **
 **********************************************************************
 */

/*
 **********************************************************************
 ** MD5.h -- Header file for implementation of MD5                   **
 ** RSA Data Security, Inc. MD5 Message Digest Algorithm             **
 ** Created: 2/17/90 RLR                                             **
 ** Revised: 12/27/90 SRD,AJ,BSK,JT Reference C version              **
 ** Revised (for MD5): RLR 4/27/91                                   **
 **   -- G modified to have y&~z instead of y&z                      **
 **   -- FF, GG, HH modified to add in last register done            **
 **   -- Access pattern: round 2 works mod 5, round 3 works mod 3    **
 **   -- distinct additive constant for each step                    **
 **   -- round 4 added, working mod 7                                **
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

#pragma once

#ifndef ___MD5_H___
#define ___MD5_H___

/* Typedef a 32 bit type */
#ifndef UINT4
typedef unsigned long int UINT4;
#endif

/* Data structure for MD5 (Message Digest) computation */
typedef struct {
	UINT4 i[2];                   /* Number of _bits_ handled mod 2^64 */
	UINT4 buf[4];                                    /* Scratch buffer */
	unsigned char in[64];                              /* Input buffer */
	unsigned char digest[16];     /* Actual digest after MD5Final call */
} MD5_CTX;

static void MD5_Transform (UINT4 *buf, UINT4 *in);

void MD5Init(MD5_CTX *mdContext, unsigned long pseudoRandomNumber = 0);
void MD5Update(MD5_CTX *mdContext, unsigned char *inBuf, unsigned int inLen);
void MD5Final(MD5_CTX *mdContext);
int md5file (char *fn , unsigned long seed , MD5_CTX *mdContext) ;

//#include "md5.cpp"

#endif /* ___MD5_H___ included */

#else

#pragma once

#ifndef ___MD5_H___
#define ___MD5_H___

#include <string>
#include <iostream>


// a small class for calculating MD5 hashes of strings or byte arrays
// it is not meant to be fast or secure
//
// usage: 1) feed it blocks of uchars with update()
//      2) finalize()
//      3) get hexdigest() string
//      or
//      MD5(std::string).hexdigest()
//
// assumes that char is 8 bit and int is 32 bit
class MD5
{
public:
    typedef unsigned int size_type; // must be 32bit

    MD5();
    MD5(const std::string& text);
    void update(const unsigned char *buf, size_type length);
    void update(const char *buf, size_type length);
    MD5& finalize();
    std::string hexdigest() const;
    friend std::ostream& operator<<(std::ostream&, MD5 md5);

private:
    void init();
    typedef unsigned char uint1; //  8bit
    typedef unsigned int uint4;  // 32bit
    enum { blocksize = 64 }; // VC6 won't eat a const static int here

    void transform(const uint1 block[blocksize]);
    static void decode(uint4 output[], const uint1 input[], size_type len);
    static void encode(uint1 output[], const uint4 input[], size_type len);

    bool finalized;
    uint1 buffer[blocksize]; // bytes that didn't fit in last 64 byte chunk
    uint4 count[2];   // 64bit counter for number of bits (lo, hi)
    uint4 state[4];   // digest so far
    uint1 digest[16]; // the result

    // low level logic operations
    static inline uint4 F(uint4 x, uint4 y, uint4 z);
    static inline uint4 G(uint4 x, uint4 y, uint4 z);
    static inline uint4 H(uint4 x, uint4 y, uint4 z);
    static inline uint4 I(uint4 x, uint4 y, uint4 z);
    static inline uint4 rotate_left(uint4 x, int n);
    static inline void FF(uint4 &a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac);
    static inline void GG(uint4 &a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac);
    static inline void HH(uint4 &a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac);
    static inline void II(uint4 &a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac);
};

std::string md5(const std::string str);

#define _NEW_MD5_LIB

#endif /* ___MD5_H___ included */

#endif