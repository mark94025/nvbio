/*
 * nvbio
 * Copyright (C) 2012-2014, NVIDIA Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#pragma once

#include <cmath>
#include <limits>
#include <nvbio/basic/types.h>
#include <nvbio/basic/iterator.h>
#include <vector_types.h>
#include <vector_functions.h>

namespace nvbio {

#define M_PIf     3.141592653589793238462643383279502884197169399375105820974944592f
#define M_PI_2f   6.283185307179586f
#define M_INV_PIf 0.3183098861837907f

#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502884197169399375105820974944592
#endif
#ifndef M_PI_2
#define M_PI_2 (2.0 * M_PI)
#endif

#if WIN32
#include <float.h>

inline bool is_finite(const double x) { return _finite(x) != 0; }
inline bool is_nan(const double x) { return _isnan(x) != 0; }
inline bool is_finite(const float x) { return _finite(x) != 0; }
inline bool is_nan(const float x) { return _isnan(x) != 0; }

#endif

#ifdef __CUDACC__

NVBIO_FORCEINLINE __device__ uint32 warp_tid() { return threadIdx.x & 31; }
NVBIO_FORCEINLINE __device__ uint32 warp_id()  { return threadIdx.x >> 5; }

#endif

///\page utilities_page Utilities
///
/// NVBIO contains various convenience functions and functors needed for every day's work:
///
/// - \ref BasicUtils
/// - \ref BasicFunctors
///

///@addtogroup Basic
///@{

///\defgroup BasicUtils Utilities
///
/// NVBIO's convenience functions and functors needed for every day's work...
///

///@addtogroup BasicUtils
///@{

namespace util
{

/// \ingroup BasicUtils
/// return the bitmask with the lo N bits set
///
template <uint32 N>
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
uint32 lo_bits() { return (1u << N) - 1u; }

/// \ingroup BasicUtils
/// return the bitmask with the hi N bits set
///
template <uint32 N>
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
uint32 hi_bits() { return ~lo_bits<N>(); }

/// \ingroup BasicUtils
/// count the number of occurrences of a given value inside an array, up to a maximum value
///
template <typename Iterator, typename T>
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
uint32 count_occurrences(const Iterator begin, uint32 size, const T val, const uint32 max_occ = uint32(-1))
{
    uint32 occ = 0u;
    for (uint32 i = 0; i < size; ++i)
    {
        if (begin[i] == val)
        {
            if (++occ >= max_occ)
                return occ;
        }
    }
    return occ;
}

/// \ingroup BasicUtils
/// x/y rounding towards +infinity for integers, used to determine # of blocks/warps etc.
///
template<typename L, typename R>
inline NVBIO_HOST_DEVICE L divide_ri(const L x, const R y)
{
    return L( (x + (y - 1)) / y );
}

/// \ingroup BasicUtils
/// x/y rounding towards zero for integers, used to determine # of blocks/warps etc.
///
template<typename L, typename R>
inline NVBIO_HOST_DEVICE L divide_rz(const L x, const R y)
{
    return L( x / y );
}

/// \ingroup BasicUtils
/// round x towards infinity to the next multiple of y
///
template<typename L, typename R>
inline NVBIO_HOST_DEVICE L round_i(const L x, const R y){ return L( y * divide_ri(x, y) ); }

/// \ingroup BasicUtils
/// round x towards zero to the next multiple of y
///
template<typename L, typename R>
inline NVBIO_HOST_DEVICE L round_z(const L x, const R y){ return L( y * divide_rz(x, y) ); }

} // end namespace util

/// return the c'th component of a by value
///
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE char comp(const char4 a, const char c)
{
    return c <= 1 ?
        (c == 0 ? a.x : a.y) :
        (c == 2 ? a.z : a.w);
}
/// return a reference to the c'th component of a
///
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE signed char& select(char4& a, const char c)
{
    return c <= 1 ?
        (c == 0 ? a.x : a.y) :
        (c == 2 ? a.z : a.w);
}

/// return the c'th component of a by value
///
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE uint32 comp(const uint4 a, const uint32 c)
{
    return c <= 1 ?
        (c == 0 ? a.x : a.y) :
        (c == 2 ? a.z : a.w);
}
/// return a reference to the c'th component of a
///
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE uint32& select(uint4& a, const uint32 c)
{
    return c <= 1 ?
        (c == 0 ? a.x : a.y) :
        (c == 2 ? a.z : a.w);
}

/// return the c'th component of a by value
///
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE uint16 comp(const ushort4 a, const uint32 c)
{
    return c <= 1 ?
        (c == 0 ? a.x : a.y) :
        (c == 2 ? a.z : a.w);
}

///@addtogroup VectorTypes
///@{

struct uint64_2
{
    uint64 x;
    uint64 y;
};
struct uint64_3
{
    uint64 x;
    uint64 y;
    uint64 z;
};
struct uint64_4
{
    uint64 x;
    uint64 y;
    uint64 z;
    uint64 w;
};
struct int64_2
{
    int64 x;
    int64 y;
};
struct int64_3
{
    int64 x;
    int64 y;
    int64 z;
};
struct int64_4
{
    int64 x;
    int64 y;
    int64 z;
    int64 w;
};

template <typename T, uint32 DIM>
struct vector_type {};

template <> struct vector_type<char,2> { typedef char2 type; NVBIO_FORCEINLINE NVBIO_HOST_DEVICE static type make(const char i1, const char i2) { return make_char2(i1,i2); } };
template <> struct vector_type<char,3> { typedef char3 type; NVBIO_FORCEINLINE NVBIO_HOST_DEVICE static type make(const char i1, const char i2, const char i3) { return make_char3(i1,i2,i3); }  };
template <> struct vector_type<char,4> { typedef char4 type; NVBIO_FORCEINLINE NVBIO_HOST_DEVICE static type make(const char i1, const char i2, const char i3, const char i4) { return make_char4(i1,i2,i3,i4); }  };

template <> struct vector_type<unsigned char,2> { typedef uchar2 type; NVBIO_FORCEINLINE NVBIO_HOST_DEVICE static type make(const unsigned char i1, const unsigned char i2) { return make_uchar2(i1,i2); } };
template <> struct vector_type<unsigned char,3> { typedef uchar3 type; NVBIO_FORCEINLINE NVBIO_HOST_DEVICE static type make(const unsigned char i1, const unsigned char i2, const unsigned char i3) { return make_uchar3(i1,i2,i3); }  };
template <> struct vector_type<unsigned char,4> { typedef uchar4 type; NVBIO_FORCEINLINE NVBIO_HOST_DEVICE static type make(const unsigned char i1, const unsigned char i2, const unsigned char i3, const unsigned char i4) { return make_uchar4(i1,i2,i3,i4); }  };

template <> struct vector_type<short,2> { typedef short2 type; NVBIO_FORCEINLINE NVBIO_HOST_DEVICE static type make(const short i1, const short i2) { return make_short2(i1,i2); } };
template <> struct vector_type<short,3> { typedef short3 type; NVBIO_FORCEINLINE NVBIO_HOST_DEVICE static type make(const short i1, const short i2, const short i3) { return make_short3(i1,i2,i3); }  };
template <> struct vector_type<short,4> { typedef short4 type; NVBIO_FORCEINLINE NVBIO_HOST_DEVICE static type make(const short i1, const short i2, const short i3, const short i4) { return make_short4(i1,i2,i3,i4); }  };

template <> struct vector_type<unsigned short,2> { typedef ushort2 type; NVBIO_FORCEINLINE NVBIO_HOST_DEVICE static type make(const unsigned short i1, const unsigned short i2) { return make_ushort2(i1,i2); } };
template <> struct vector_type<unsigned short,3> { typedef ushort3 type; NVBIO_FORCEINLINE NVBIO_HOST_DEVICE static type make(const unsigned short i1, const unsigned short i2, const unsigned short i3) { return make_ushort3(i1,i2,i3); }  };
template <> struct vector_type<unsigned short,4> { typedef ushort4 type; NVBIO_FORCEINLINE NVBIO_HOST_DEVICE static type make(const unsigned short i1, const unsigned short i2, const unsigned short i3, const unsigned short i4) { return make_ushort4(i1,i2,i3,i4); }  };

template <> struct vector_type<int,2> { typedef int2 type; NVBIO_FORCEINLINE NVBIO_HOST_DEVICE static type make(const int i1, const int i2) { return make_int2(i1,i2); } };
template <> struct vector_type<int,3> { typedef int3 type; NVBIO_FORCEINLINE NVBIO_HOST_DEVICE static type make(const int i1, const int i2, const int i3) { return make_int3(i1,i2,i3); }  };
template <> struct vector_type<int,4> { typedef int4 type; NVBIO_FORCEINLINE NVBIO_HOST_DEVICE static type make(const int i1, const int i2, const int i3, const int i4) { return make_int4(i1,i2,i3,i4); }  };

template <> struct vector_type<unsigned int,2> { typedef uint2 type; NVBIO_FORCEINLINE NVBIO_HOST_DEVICE static type make(const unsigned int i1, const unsigned int i2) { return make_uint2(i1,i2); } };
template <> struct vector_type<unsigned int,3> { typedef uint3 type; NVBIO_FORCEINLINE NVBIO_HOST_DEVICE static type make(const unsigned int i1, const unsigned int i2, const unsigned int i3) { return make_uint3(i1,i2,i3); }  };
template <> struct vector_type<unsigned int,4> { typedef uint4 type; NVBIO_FORCEINLINE NVBIO_HOST_DEVICE static type make(const unsigned int i1, const unsigned int i2, const unsigned int i3, const unsigned int i4) { return make_uint4(i1,i2,i3,i4); }  };

template <> struct vector_type<int64,2>  { typedef  int64_2 type; NVBIO_FORCEINLINE NVBIO_HOST_DEVICE static type make(const int64 i1, const int64 i2)                                 {  int64_2 r; r.x = i1; r.y = i2; return r; } };
template <> struct vector_type<int64,3>  { typedef  int64_3 type; NVBIO_FORCEINLINE NVBIO_HOST_DEVICE static type make(const int64 i1, const int64 i2, const int64 i3)                 {  int64_3 r; r.x = i1; r.y = i2; r.z = i3; return r; } };
template <> struct vector_type<int64,4>  { typedef  int64_4 type; NVBIO_FORCEINLINE NVBIO_HOST_DEVICE static type make(const int64 i1, const int64 i2, const int64 i3, const int64 i4) {  int64_4 r; r.x = i1; r.y = i2; r.z = i3, r.w = i4; return r; } };

template <> struct vector_type<uint64,2>  { typedef uint64_2 type; NVBIO_FORCEINLINE NVBIO_HOST_DEVICE static type make(const uint64 i1, const uint64 i2)                                   { uint64_2 r; r.x = i1; r.y = i2; return r; } };
template <> struct vector_type<uint64,3>  { typedef uint64_3 type; NVBIO_FORCEINLINE NVBIO_HOST_DEVICE static type make(const uint64 i1, const uint64 i2, const uint64 i3)                  { uint64_3 r; r.x = i1; r.y = i2; r.z = i3; return r; } };
template <> struct vector_type<uint64,4>  { typedef uint64_4 type; NVBIO_FORCEINLINE NVBIO_HOST_DEVICE static type make(const uint64 i1, const uint64 i2, const uint64 i3, const uint64 i4) { uint64_4 r; r.x = i1; r.y = i2; r.z = i3, r.w = i4; return r; } };

template <> struct vector_type<float,2> { typedef float2 type; NVBIO_FORCEINLINE NVBIO_HOST_DEVICE static type make(const float i1, const float i2) { return make_float2(i1,i2); } };
template <> struct vector_type<float,3> { typedef float3 type; NVBIO_FORCEINLINE NVBIO_HOST_DEVICE static type make(const float i1, const float i2, const float i3) { return make_float3(i1,i2,i3); }  };
template <> struct vector_type<float,4> { typedef float4 type; NVBIO_FORCEINLINE NVBIO_HOST_DEVICE static type make(const float i1, const float i2, const float i3, const float i4) { return make_float4(i1,i2,i3,i4); }  };

template <typename T> NVBIO_FORCEINLINE NVBIO_HOST_DEVICE typename vector_type<T,2>::type make_vector(const T i1, const T i2)                         { return vector_type<T,2>::make( i1, i2 ); }
template <typename T> NVBIO_FORCEINLINE NVBIO_HOST_DEVICE typename vector_type<T,3>::type make_vector(const T i1, const T i2, const T i3)             { return vector_type<T,3>::make( i1, i2, i3 ); }
template <typename T> NVBIO_FORCEINLINE NVBIO_HOST_DEVICE typename vector_type<T,4>::type make_vector(const T i1, const T i2, const T i3, const T i4) { return vector_type<T,4>::make( i1, i2, i3, i4 ); }

template <typename T> struct vector_traits {};
template <>           struct vector_traits<char>           { typedef          char  value_type; const static uint32 DIM = 1; };
template <>           struct vector_traits<unsigned char>  { typedef unsigned char  value_type; const static uint32 DIM = 1; };
template <>           struct vector_traits<short>          { typedef          short value_type; const static uint32 DIM = 1; };
template <>           struct vector_traits<unsigned short> { typedef unsigned short value_type; const static uint32 DIM = 1; };
template <>           struct vector_traits<int>            { typedef          int   value_type; const static uint32 DIM = 1; };
template <>           struct vector_traits<unsigned int>   { typedef unsigned int   value_type; const static uint32 DIM = 1; };
template <>           struct vector_traits<int64>          { typedef          int64 value_type; const static uint32 DIM = 1; };
template <>           struct vector_traits<uint64>         { typedef         uint64 value_type; const static uint32 DIM = 1; };
template <>           struct vector_traits<float>          { typedef         float  value_type; const static uint32 DIM = 1; };
template <>           struct vector_traits<double>         { typedef         double value_type; const static uint32 DIM = 1; };
template <>           struct vector_traits<char2>   { typedef char value_type; const static uint32 DIM = 2; };
template <>           struct vector_traits<char3>   { typedef char value_type; const static uint32 DIM = 3; };
template <>           struct vector_traits<char4>   { typedef char value_type; const static uint32 DIM = 4; };
template <>           struct vector_traits<uchar2>  { typedef unsigned char value_type; const static uint32 DIM = 2; };
template <>           struct vector_traits<uchar3>  { typedef unsigned char value_type; const static uint32 DIM = 3; };
template <>           struct vector_traits<uchar4>  { typedef unsigned char value_type; const static uint32 DIM = 4; };
template <>           struct vector_traits<short2>  { typedef short value_type; const static uint32 DIM = 2; };
template <>           struct vector_traits<short3>  { typedef short value_type; const static uint32 DIM = 3; };
template <>           struct vector_traits<short4>  { typedef short value_type; const static uint32 DIM = 4; };
template <>           struct vector_traits<ushort2> { typedef unsigned short value_type; const static uint32 DIM = 2; };
template <>           struct vector_traits<ushort3> { typedef unsigned short value_type; const static uint32 DIM = 3; };
template <>           struct vector_traits<ushort4> { typedef unsigned short value_type; const static uint32 DIM = 4; };
template <>           struct vector_traits<int2>    { typedef int value_type; const static uint32 DIM = 2; };
template <>           struct vector_traits<int3>    { typedef int value_type; const static uint32 DIM = 3; };
template <>           struct vector_traits<int4>    { typedef int value_type; const static uint32 DIM = 4; };
template <>           struct vector_traits<uint2>   { typedef unsigned int value_type; const static uint32 DIM = 2; };
template <>           struct vector_traits<uint3>   { typedef unsigned int value_type; const static uint32 DIM = 3; };
template <>           struct vector_traits<uint4>   { typedef unsigned int value_type; const static uint32 DIM = 4; };
template <>           struct vector_traits<float2>  { typedef float value_type; const static uint32 DIM = 2; };
template <>           struct vector_traits<float3>  { typedef float value_type; const static uint32 DIM = 3; };
template <>           struct vector_traits<float4>  { typedef float value_type; const static uint32 DIM = 4; };
template <>           struct vector_traits<uint64_2> { typedef uint64 value_type; const static uint32 DIM = 2; };
template <>           struct vector_traits<uint64_3> { typedef uint64 value_type; const static uint32 DIM = 3; };
template <>           struct vector_traits<uint64_4> { typedef uint64 value_type; const static uint32 DIM = 4; };
template <>           struct vector_traits<int64_2>  { typedef int64  value_type; const static uint32 DIM = 2; };
template <>           struct vector_traits<int64_3>  { typedef int64  value_type; const static uint32 DIM = 3; };
template <>           struct vector_traits<int64_4>  { typedef int64  value_type; const static uint32 DIM = 4; };

///@} VectorTypes

/// sign function
///
template <typename T>
inline NVBIO_HOST_DEVICE T sgn(const T x) { return x > 0 ? T(1) : T(-1); }

/// round a floating point number
///
inline NVBIO_HOST_DEVICE float round(const float x)
{
	const int y = x > 0.0f ? int(x) : int(x)-1;
	return (x - float(y) > 0.5f) ? float(y)+1.0f : float(y);
}

/// minimum of two floats
///
inline NVBIO_HOST_DEVICE float min(const float a, const float b) { return a < b ? a : b; }

/// maximum of two floats
///
inline NVBIO_HOST_DEVICE float max(const float a, const float b) { return a > b ? a : b; }

/// minimum of two int8
///
inline NVBIO_HOST_DEVICE int8 min(const int8 a, const int8 b) { return a < b ? a : b; }

/// maximum of two int8
///
inline NVBIO_HOST_DEVICE int8 max(const int8 a, const int8 b) { return a > b ? a : b; }

/// minimum of two uint8
///
inline NVBIO_HOST_DEVICE uint8 min(const uint8 a, const uint8 b) { return a < b ? a : b; }

/// maximum of two uint8
///
inline NVBIO_HOST_DEVICE uint8 max(const uint8 a, const uint8 b) { return a > b ? a : b; }

/// minimum of two int32
///
inline NVBIO_HOST_DEVICE int32 min(const int32 a, const int32 b) { return a < b ? a : b; }

/// maximum of two int32
///
inline NVBIO_HOST_DEVICE int32 max(const int32 a, const int32 b) { return a > b ? a : b; }

/// minimum of two uint32
///
inline NVBIO_HOST_DEVICE uint32 min(const uint32 a, const uint32 b) { return a < b ? a : b; }

/// maximum of two uint32
///
inline NVBIO_HOST_DEVICE uint32 max(const uint32 a, const uint32 b) { return a > b ? a : b; }

/// minimum of two int64
///
inline NVBIO_HOST_DEVICE int64 min(const int64 a, const int64 b) { return a < b ? a : b; }

/// maximum of two int64
///
inline NVBIO_HOST_DEVICE int64 max(const int64 a, const int64 b) { return a > b ? a : b; }

/// minimum of two uint64
///
inline NVBIO_HOST_DEVICE uint64 min(const uint64 a, const uint64 b) { return a < b ? a : b; }

/// maximum of two uint64
///
inline NVBIO_HOST_DEVICE uint64 max(const uint64 a, const uint64 b) { return a > b ? a : b; }

/// quantize the float x in [0,1] to an integer [0,...,n[
///
inline NVBIO_HOST_DEVICE uint32 quantize(const float x, const uint32 n)
{
	return (uint32)max( min( int32( x * float(n) ), int32(n-1) ), int32(0) );
}
/// compute the floating point module of a quantity with sign
///
inline float NVBIO_HOST_DEVICE mod(const float x, const float m) { return x > 0.0f ? fmodf( x, m ) : m - fmodf( -x, m ); }

/// compute the log base 2 of an integer
///
inline NVBIO_HOST_DEVICE uint32 log2(uint32 n)
{
    unsigned int c = 0;
    if (n & 0xffff0000u) { n >>= 16; c |= 16; }
    if (n & 0xff00) { n >>= 8; c |= 8; }
    if (n & 0xf0) { n >>= 4; c |= 4; }
    if (n & 0xc) { n >>= 2; c |= 2; }
    if (n & 0x2) c |= 1;
    return c;
/*    uint32 m = 0;
    while (n > 0)
    {
        n >>= 1;
        m++;
    }
    return m-1;*/
}

/// compute a simple 32-bit hash
///
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE 
uint32 hash(uint32 a)
{
    a = (a+0x7ed55d16) + (a<<12);
    a = (a^0xc761c23c) ^ (a>>19);
    a = (a+0x165667b1) + (a<<5);
    a = (a+0xd3a2646c) ^ (a<<9);
    a = (a+0xfd7046c5) + (a<<3);
    a = (a^0xb55a4f09) ^ (a>>16);
    return a;
}

/// Thomas Wang's 32 bit Mix Function: http://www.cris.com/~Ttwang/tech/inthash.htm
///
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE 
uint32 hash2(uint32 key)
{
    key += ~(key << 15);
    key ^=  (key >> 10);
    key +=  (key << 3);
    key ^=  (key >> 6);
    key += ~(key << 11);
    key ^=  (key >> 16);
    return key;
}

/// Thomas Wang's 64 bit Mix Function: http://www.cris.com/~Ttwang/tech/inthash.htm
///
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE 
uint64 hash(uint64 key)
{
    key += ~(key << 32);
    key ^=  (key >> 22);
    key += ~(key << 13);
    key ^=  (key >> 8);
    key +=  (key << 3);
    key ^=  (key >> 15);
    key += ~(key << 27);
    key ^=  (key >> 31);
    return key;
}

/// simple 64-bit hash
///
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE 
uint64 hash2(uint64 key)
{
    return (key >> 32) ^ key;
}

/// elf 64-bit hash
///
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE 
uint64 hash3(uint64 key)
{
    uint32 hash = 0u;

    #if defined(__CUDA_ARCH__)
    #pragma unroll
    #endif
    for (uint32 i = 0; i < 8; ++i)
    {
        hash = (hash << 4) + ((key >> (i*8)) & 255u); // shift/mix

        // get high nybble
        const uint32 hi_bits = hash & 0xF0000000;
        if (hi_bits != 0u)
            hash ^= hi_bits >> 24; // xor high nybble with second nybble

        hash &= ~hi_bits; // clear high nybble
    }
    return hash;
}

#define NVBIO_RAND_A 1664525
#define NVBIO_RAND_C 1013904223

/// A very simple Linear Congruential Generator
///
struct LCG_random
{
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE LCG_random(const uint32 s = 0) : m_s(s) {}

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE uint32 next() { m_s = m_s*NVBIO_RAND_A + NVBIO_RAND_C; return m_s; }

    uint32 m_s;
};

#if defined(__CUDA_ARCH__)

NVBIO_FORCEINLINE NVBIO_DEVICE 
uint8 min3(const uint8 op1, const uint8 op2, const uint8 op3)
{
    uint32 r;
    asm( "  vmin.u32.u32.u32.min %0, %1, %2, %3;"               : "=r"(r)                              : "r"(uint32(op1)), "r"(uint32(op2)), "r"(uint32(op3)) );
    return r;
}

NVBIO_FORCEINLINE NVBIO_DEVICE 
uint32 min3(const uint32 op1, const uint32 op2, const uint32 op3)
{
    uint32 r;
    asm( "  vmin.u32.u32.u32.min %0, %1, %2, %3;"               : "=r"(r)                              : "r"(op1), "r"(op2), "r"(op3) );
    return r;
}

NVBIO_FORCEINLINE NVBIO_DEVICE 
uint32 max3(const uint32 op1, const uint32 op2, const uint32 op3)
{
    uint32 r;
    asm( "  vmax.u32.u32.u32.max %0, %1, %2, %3;"               : "=r"(r)                              : "r"(op1), "r"(op2), "r"(op3) );
    return r;
}

NVBIO_FORCEINLINE NVBIO_DEVICE 
int32 min3(const int32 op1, const int32 op2, const int32 op3)
{
    uint32 r;
    asm( "  vmin.s32.s32.s32.min %0, %1, %2, %3;"               : "=r"(r)                              : "r"(op1), "r"(op2), "r"(op3) );
    return r;
}

NVBIO_FORCEINLINE NVBIO_DEVICE 
int32 max3(const int32 op1, const int32 op2, const int32 op3)
{
    uint32 r;
    asm( "  vmax.s32.s32.s32.max %0, %1, %2, %3;"               : "=r"(r)                              : "r"(op1), "r"(op2), "r"(op3) );
    return r;
}

#else

NVBIO_FORCEINLINE NVBIO_HOST_DEVICE 
uint8 min3(const uint8 op1, const uint8 op2, const uint8 op3)
{
    return nvbio::min( op1, nvbio::min( op2, op3 ) );
}

NVBIO_FORCEINLINE NVBIO_HOST_DEVICE 
uint32 min3(const uint32 op1, const uint32 op2, const uint32 op3)
{
    return nvbio::min( op1, nvbio::min( op2, op3 ) );
}

NVBIO_FORCEINLINE NVBIO_HOST_DEVICE 
uint32 max3(const uint32 op1, const uint32 op2, const uint32 op3)
{
    return nvbio::max( op1, nvbio::max( op2, op3 ) );
}
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE 
int32 min3(const int32 op1, const int32 op2, const int32 op3)
{
    return nvbio::min( op1, nvbio::min( op2, op3 ) );
}

NVBIO_FORCEINLINE NVBIO_HOST_DEVICE 
int32 max3(const int32 op1, const int32 op2, const int32 op3)
{
    return nvbio::max( op1, nvbio::max( op2, op3 ) );
}

#endif

NVBIO_FORCEINLINE NVBIO_HOST_DEVICE 
float min3(const float op1, const float op2, const float op3)
{
    return nvbio::min( op1, nvbio::min( op2, op3 ) );
}

NVBIO_FORCEINLINE NVBIO_HOST_DEVICE 
float max3(const float op1, const float op2, const float op3)
{
    return nvbio::max( op1, nvbio::max( op2, op3 ) );
}

#ifdef __CUDA_ARCH__

inline NVBIO_DEVICE float fast_pow(const float a, const float b)
{
    return __powf(a,b);
}
inline NVBIO_DEVICE float fast_sin(const float x)
{
    return __sinf(x);
}
inline NVBIO_DEVICE float fast_cos(const float x)
{
    return __cosf(x);
}
inline NVBIO_DEVICE float fast_sqrt(const float x)
{
    return __fsqrt_rn(x);
}

#else

inline NVBIO_HOST_DEVICE float fast_pow(const float a, const float b)
{
    return ::powf(a,b);
}
inline NVBIO_HOST_DEVICE float fast_sin(const float x)
{
    return sinf(x);
}
inline NVBIO_HOST_DEVICE float fast_cos(const float x)
{
    return cosf(x);
}
inline NVBIO_HOST_DEVICE float fast_sqrt(const float x)
{
    return sqrtf(x);
}

#endif

#ifdef __CUDACC__
inline NVBIO_DEVICE uint16 float_to_half(const float x) { return __float2half_rn(x); }
inline NVBIO_DEVICE float  half_to_float(const uint32 h) { return __half2float(h); }
#endif

///
/// A generic class to represent traits of numeric types T.
/// Unlike STL's numeric_traits, Field_traits<T>::min() and Field_traits<T>::max() are signed.
///
template <typename T>
struct Field_traits
{
#ifdef __CUDACC__
    /// return the minimum value of T
    ///
	NVBIO_HOST_DEVICE static T min() { return T(); }

    /// return the maximum value of T
    ///
    NVBIO_HOST_DEVICE static T max() { return T(); }
#else
    /// return the minimum value of T
    ///
	static T min()
    {
        return std::numeric_limits<T>::is_integer ?
             std::numeric_limits<T>::min() :
            -std::numeric_limits<T>::max();
    }
    /// return the maximum value of T
    ///
	static T max() { return std::numeric_limits<T>::max(); }
#endif
};

/// int8 specialization of Field_traits
///
template <>
struct Field_traits<int8>
{
	NVBIO_HOST_DEVICE static int8 min() { return -128; }
    NVBIO_HOST_DEVICE static int8 max() { return  127; }
};
/// int16 specialization of Field_traits
///
template <>
struct Field_traits<int16>
{
	NVBIO_HOST_DEVICE static int16 min() { return -32768; }
    NVBIO_HOST_DEVICE static int16 max() { return  32767; }
};
/// int32 specialization of Field_traits
///
template <>
struct Field_traits<int32>
{
	NVBIO_HOST_DEVICE static int32 min() { return -(1 << 30); }
    NVBIO_HOST_DEVICE static int32 max() { return  (1 << 30); }
};
/// int64 specialization of Field_traits
///
template <>
struct Field_traits<int64>
{
	NVBIO_HOST_DEVICE static int64 min() { return -(int64(1) << 62); }
    NVBIO_HOST_DEVICE static int64 max() { return  (int64(1) << 62); }
};

#ifdef __CUDACC__
/// float specialization of Field_traits
///
template <>
struct Field_traits<float>
{
	NVBIO_HOST_DEVICE static float min() { return -float(1.0e+30f); }
    NVBIO_HOST_DEVICE static float max() { return  float(1.0e+30f); }
};
/// double specialization of Field_traits
///
template <>
struct Field_traits<double>
{
	NVBIO_HOST_DEVICE static double min() { return -double(1.0e+30); }
    NVBIO_HOST_DEVICE static double max() { return  double(1.0e+30); }
};
/// uint32 specialization of Field_traits
///
template <>
struct Field_traits<uint32>
{
	NVBIO_HOST_DEVICE static uint32 min() { return 0; }
    NVBIO_HOST_DEVICE static uint32 max() { return uint32(-1); }
};
/// uint64 specialization of Field_traits
///
template <>
struct Field_traits<uint64>
{
	NVBIO_HOST_DEVICE static uint64 min() { return 0; }
    NVBIO_HOST_DEVICE static uint64 max() { return uint64(-1); }
};
#endif

///\defgroup BasicFunctors Functors
///
/// NVBIO's convenience functors needed for every day's work...
///

///@addtogroup BasicFunctors
///@{

/// constant functor
///
template <typename T>
struct Constant
{
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    Constant(const T k) : m_k(k) {}

    template <typename U>
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    T operator() (const U x) const { return m_k; }

    const T m_k;
};

/// linear functor
///
template <typename T>
struct Linear
{
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    Linear(const T m) : m_m(m) {}

    template <typename U>
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    T operator() (const U x) const { return T(m_m * x); }

    const T m_m;
};

/// affine functor
///
template <typename T>
struct Affine
{
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    Affine(const T k, const T m) : m_k(k), m_m(m) {}

    template <typename U>
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    T operator() (const U x) const { return m_k + T(m_m * x); }

    const T m_k;
    const T m_m;
};

/// add functor
///
struct add_functor
{
    template <typename T>
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    T operator() (const T op1, const T op2) const { return op1 + op2; }
};
/// min functor
///
struct min_functor
{
    template <typename T>
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    T operator() (const T op1, const T op2) const { return nvbio::min( op1, op2 ); }
};
/// max functor
///
struct max_functor
{
    template <typename T>
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    T operator() (const T op1, const T op2) const { return nvbio::max( op1, op2 ); }
};

/// A functor to take the n leading bits of a word
///
template <typename word_type>
struct leading_bits
{
    typedef word_type argument_type;
    typedef word_type result_type;

    static const uint32 BITS = 8u*sizeof(word_type);

    /// constructor
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    leading_bits(const uint32 n) : n_bits( n ) {}

    /// functor implementation
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    result_type operator() (const argument_type op) const { return op >> (BITS - n_bits); }

    const uint32 n_bits;
};

/// Return the first M hi-bits of an N-bit word
///
template <typename T, typename U>
struct hi_bits_functor {};

/// Return the first 8 hi-bits of a 32-bit word
///
template <>
struct hi_bits_functor<uint8,uint32>
{
    typedef uint32 argument_type;
    typedef uint8  result_type;

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    result_type operator() (const argument_type op) const { return result_type( op >> 24u ); }
};

/// Return the first 16 hi-bits of a 32-bit word
///
template <>
struct hi_bits_functor<uint16,uint32>
{
    typedef uint32 argument_type;
    typedef uint16 result_type;

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    result_type operator() (const argument_type op) const { return result_type( op >> 16u ); }
};

/// Return the first 32 hi-bits of a 32-bit word
///
template <>
struct hi_bits_functor<uint32,uint32>
{
    typedef uint32 argument_type;
    typedef uint32 result_type;

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    result_type operator() (const argument_type op) const { return result_type( op ); }
};

/// Get a given character from a vector
///
template <typename T>
struct get_char_functor
{
    typedef T    argument_type;
    typedef char result_type;

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    get_char_functor(const uint32 i) : m_i( i ) {}

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    result_type operator() (const argument_type op) const { return result_type( op[m_i] ); }

    uint32 m_i;
};

/// A unary functor returning true if op evaluates to true
///
template <typename T>
struct is_true_functor
{
    typedef T    argument_type;
    typedef bool result_type;

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    result_type operator() (const T op) const { return op ? true : false; }
};

/// A unary functor returning true if op evaluates to false
///
template <typename T>
struct is_false_functor
{
    typedef T    argument_type;
    typedef bool result_type;

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    result_type operator() (const T op) const { return op ? false : true; }
};

/// A unary functor returning true if op == K
///
template <typename T>
struct equal_to_functor
{
    typedef T    argument_type;
    typedef bool result_type;

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    equal_to_functor(const T k) : m_k( k ) {}

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    result_type operator() (const T op) const { return op == m_k; }

    const T m_k;
};

/// A binary functor returning true if op1 == op2
///
template <typename T>
struct equal_functor
{
    typedef T    first_argument_type;
    typedef T    second_argument_type;
    typedef bool result_type;

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    result_type operator() (const T op1, const T op2) const { return op1 == op2; }
};

/// A binary functor returning true if op1 != op2
///
template <typename T>
struct not_equal_functor
{
    typedef T    first_argument_type;
    typedef T    second_argument_type;
    typedef bool result_type;

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    result_type operator() (const T op1, const T op2) const { return op1 != op2; }
};

/// Build a gathering functor
///
template <typename Iterator, typename index_type = uint32>
struct gather_functor
{
    typedef index_type                                              argument_type;
    typedef typename std::iterator_traits<Iterator>::value_type  result_type;

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    gather_functor(const Iterator perm) : m_perm( perm ) {}

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    result_type operator() (const argument_type op) const { return m_perm[ op ]; }

    Iterator m_perm;
};
template <typename Iterator>
gather_functor<Iterator> make_gather_functor(const Iterator perm)
{
    return gather_functor<Iterator>( perm );
}

/// Compose two functors
///
template <typename Functor2, typename Functor1>
struct composition_functor
{
    typedef typename Functor1::argument_type   argument_type;
    typedef typename Functor2::result_type     result_type;

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    composition_functor(const Functor2 fun2, const Functor1 fun1) : m_fun1( fun1 ), m_fun2( fun2 ) {}

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    result_type operator() (const argument_type op) const { return m_fun2( m_fun1( op ) ); }

    Functor1    m_fun1;
    Functor2    m_fun2;
};
template <typename Functor2, typename Functor1>
composition_functor<Functor2,Functor1> make_composition_functor(const Functor2 fun2, const Functor1 fun1)
{
    return composition_functor<Functor2,Functor1>( fun2, fun1 );
}

/// Bind the first argument of a functor
///
template <typename Functor>
struct bind_first_functor
{
    typedef typename Functor::second_argument_type   argument_type;
    typedef typename Functor::first_argument_type    const_type;
    typedef typename Functor::result_type            result_type;

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    bind_first_functor(const const_type c) : m_fun(), m_c( c ) {}

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    bind_first_functor(const Functor fun, const const_type c) : m_fun( fun ), m_c( c ) {}

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    result_type operator() (const argument_type op) const { return m_fun( m_c, op ); }

    Functor     m_fun;
    const_type  m_c;
};

/// Bind the second argument of a functor
///
template <typename Functor>
struct bind_second_functor
{
    typedef typename Functor::first_argument_type    argument_type;
    typedef typename Functor::second_argument_type   const_type;
    typedef typename Functor::result_type            result_type;

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    bind_second_functor(const const_type c) : m_fun(), m_c( c ) {}

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    bind_second_functor(const Functor fun, const const_type c) : m_fun( fun ), m_c( c ) {}

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    result_type operator() (const argument_type op) const { return m_fun( op, m_c ); }

    Functor     m_fun;
    const_type  m_c;
};

/// helper functor to reverse a sequence
///
template <typename IndexType>
struct reverse_functor
{
    typedef IndexType index_type;

    /// empty constructor
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    reverse_functor() {}

    /// constructor
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    reverse_functor(const index_type len) : m_len( len ) {}

    /// functor operator
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    index_type operator() (const index_type i) const { return m_len - i - 1; }

    index_type m_len;
};

/// helper functor to complement a sequence
///
/// \tparam ALPHABET_SIZE   size of the alphabet - only characters
///                         c: c < ALPHABET_SIZE do get complemented
///
template <uint32 ALPHABET_SIZE>
struct complement_functor
{
    /// empty constructor
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    complement_functor() {}

    /// functor operator
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    uint8 operator() (const uint8 c) const { return c >= ALPHABET_SIZE ? c : uint8(ALPHABET_SIZE-1) - c; }
};

///@} BasicFunctors
///@} BasicUtils
///@} Basic

} // namespace nvbio
