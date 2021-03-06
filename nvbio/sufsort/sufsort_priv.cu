/*
 * nvbio
 * Copyright (C) 2011-2014, NVIDIA Corporation
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

#include <nvbio/sufsort/sufsort_priv.h>
#ifdef _OPENMP
#include <omp.h>
#endif

namespace nvbio {
namespace priv {

// pack a set of head flags into a bit-packed array
//
__global__ void pack_flags_kernel(
    const uint32            n,
    const uint8*            flags,
          uint32*           comp_flags)
{
    const uint32 thread_id = (threadIdx.x + blockIdx.x * blockDim.x);
    const uint32 idx = 32 * thread_id;
    if (idx >= n)
        return;

    // initialize the output word
    uint32 f = 0u;

    #pragma unroll
    for (uint32 i = 0; i < 2; ++i)
    {
        // fetch and process 16-bytes in one go
        const uint4 flag = ((const uint4*)flags)[idx/16u + i];

        if (flag.x & (255u <<  0)) f |= 1u << (i*16 + 0);
        if (flag.x & (255u <<  8)) f |= 1u << (i*16 + 1);
        if (flag.x & (255u << 16)) f |= 1u << (i*16 + 2);
        if (flag.x & (255u << 24)) f |= 1u << (i*16 + 3);
        if (flag.y & (255u <<  0)) f |= 1u << (i*16 + 4);
        if (flag.y & (255u <<  8)) f |= 1u << (i*16 + 5);
        if (flag.y & (255u << 16)) f |= 1u << (i*16 + 6);
        if (flag.y & (255u << 24)) f |= 1u << (i*16 + 7);
        if (flag.z & (255u <<  0)) f |= 1u << (i*16 + 8);
        if (flag.z & (255u <<  8)) f |= 1u << (i*16 + 9);
        if (flag.z & (255u << 16)) f |= 1u << (i*16 + 10);
        if (flag.z & (255u << 24)) f |= 1u << (i*16 + 11);
        if (flag.w & (255u <<  0)) f |= 1u << (i*16 + 12);
        if (flag.w & (255u <<  8)) f |= 1u << (i*16 + 13);
        if (flag.w & (255u << 16)) f |= 1u << (i*16 + 14);
        if (flag.w & (255u << 24)) f |= 1u << (i*16 + 15);
    }

    // write the output word
    comp_flags[thread_id] = f;
}

// pack a set of head flags into a bit-packed array
//
void pack_flags(
    const uint32            n,
    const uint8*            flags,
          uint32*           comp_flags)
{
    const uint32 blockdim = 128;
    const uint32 n_words  = (n + 31) / 32;
    const uint32 n_blocks = (n_words + blockdim-1) / blockdim;

    pack_flags_kernel<<<n_blocks,blockdim>>>(
        n,
        flags,
        comp_flags );
}

// build a set of head flags looking at adjacent keys
//
__global__ void build_head_flags_kernel(
    const uint32            n,
    const uint32*           keys,
          uint8*            flags)
{
    const uint32 thread_id = (threadIdx.x + blockIdx.x * blockDim.x);
    const uint32 idx = 4 * thread_id;
    if (idx >= n)
        return;

    // load the previous key
    const uint32 key_p = idx ? keys[idx-1] : 0xFFFFFFFF;

    // load the next 4 keys
    const uint4  key  = ((const uint4*)keys)[thread_id];
    const uchar4 flag = ((const uchar4*)flags)[thread_id];

    // and write the corresponding 4 flags
    ((uchar4*)flags)[thread_id] = make_uchar4(
        (key.x != key_p) ? 1u : flag.x,
        (key.y != key.x) ? 1u : flag.y,
        (key.z != key.y) ? 1u : flag.z,
        (key.w != key.z) ? 1u : flag.w );
}

// build a set of head flags looking at adjacent keys
//
void build_head_flags(
    const uint32            n,
    const uint32*           keys,
          uint8*            flags)
{
    const uint32 blockdim = 128;
    const uint32 n_blocks = ((n+3)/4 + blockdim-1) / blockdim;

    build_head_flags_kernel<<<n_blocks,blockdim>>>(
        n,
        keys,
        flags );
}

// build a set of head flags looking at adjacent keys
//
__global__ void build_head_flags_kernel(
    const uint32            n,
    const uint64*           keys,
          uint8*            flags)
{
    const uint32 thread_id = (threadIdx.x + blockIdx.x * blockDim.x);
    const uint32 idx = thread_id;
    if (idx >= n)
        return;

    // load the previous key
    const uint64 key_p = idx ? keys[idx-1] : 0xFFFFFFFF;

    // load the next key
    const uint64 key = keys[thread_id];
    const uint8 flag = flags[thread_id];

    // and write the corresponding flag out
    flags[thread_id] = (key != key_p) ? 1u : flag;
}

// build a set of head flags looking at adjacent keys
//
void build_head_flags(
    const uint32            n,
    const uint64*           keys,
          uint8*            flags)
{
    const uint32 blockdim = 128;
    const uint32 n_blocks = (n + blockdim-1) / blockdim;

    build_head_flags_kernel<<<n_blocks,blockdim>>>(
        n,
        keys,
        flags );
}

uint32 extract_radix_16(
    const priv::string_set_2bit_be& string_set,
    const uint2                  suffix,
    const uint32                 word_idx)
{
    priv::local_set_suffix_word_functor<2u,16u,4u,priv::string_set_2bit_be,uint32> word_functor( string_set, word_idx );
    return word_functor( suffix );
}

uint32 extract_radix_32(
    const priv::string_set_2bit_be& string_set,
    const uint2                  suffix,
    const uint32                 word_idx)
{
    priv::local_set_suffix_word_functor<2u,32u,4u,priv::string_set_2bit_be,uint32> word_functor( string_set, word_idx );
    return word_functor( suffix );
}

uint32 extract_radix_64(
    const priv::string_set_2bit_u64_be& string_set,
    const uint2                         suffix,
    const uint32                        word_idx)
{
    priv::local_set_suffix_word_functor<2u,64u,5u,priv::string_set_2bit_u64_be,uint32> word_functor( string_set, word_idx );
    return word_functor( suffix );
}

void extract_radices_16(
    const priv::string_set_2bit_be  string_set,
    const uint32                    n_suffixes,
    const uint32                    word_begin,
    const uint32                    word_end,
    const uint2*                    h_suffixes,
          uint32*                   h_radices,
          uint8*                    h_symbols)
{
    if (word_begin+1 == word_end)
    {
        #pragma omp parallel for
        for (int32 i = 0; i < int32( n_suffixes ); ++i)
        {
            const uint2 suffix = h_suffixes[ i ];
            h_radices[i] = extract_radix_16( string_set, suffix, word_begin );
        }
    }
    else
    {
        #pragma omp parallel for
        for (int32 i = 0; i < int32( n_suffixes ); ++i)
        {
            const uint2 local_suffix_idx = h_suffixes[ i ];

            const uint32 string_idx = local_suffix_idx.y;
            const uint32 suffix_idx = local_suffix_idx.x;

            const uint64 string_off = string_set.offsets()[ string_idx ];
            const uint64 string_end = string_set.offsets()[ string_idx+1u ];
            const uint64 string_len = uint32( string_end - string_off );

            const uint32* base_words = string_set.base_string().container().stream();

            if (h_symbols != NULL)
                h_symbols[i] = suffix_idx ? string_set.base_string()[ string_off + suffix_idx-1u ] : 255u;

            extract_word_packed<16u,4u,2u>(
                base_words,
                string_len,
                string_off,
                suffix_idx,
                word_begin,
                word_end,
                strided_iterator<uint32*>( h_radices + i, n_suffixes ) );
        }
    }
}
void extract_radices_32(
    const priv::string_set_2bit_be  string_set,
    const uint32                    n_suffixes,
    const uint32                    word_begin,
    const uint32                    word_end,
    const uint2*                    h_suffixes,
          uint32*                   h_radices,
          uint8*                    h_symbols)
{
    if (word_begin+1 == word_end)
    {
        #pragma omp parallel for
        for (int32 i = 0; i < int32( n_suffixes ); ++i)
        {
            const uint2 suffix = h_suffixes[ i ];
            h_radices[i] = extract_radix_32( string_set, suffix, word_begin );
        }
    }
    else
    {
        #pragma omp parallel for
        for (int32 i = 0; i < int32( n_suffixes ); ++i)
        {
            const uint2 local_suffix_idx = h_suffixes[ i ];

            const uint32 string_idx = local_suffix_idx.y;
            const uint32 suffix_idx = local_suffix_idx.x;

            const uint64 string_off = string_set.offsets()[ string_idx ];
            const uint64 string_end = string_set.offsets()[ string_idx+1u ];
            const uint64 string_len = uint32( string_end - string_off );

            const uint32* base_words = string_set.base_string().container().stream();

            if (h_symbols != NULL)
                h_symbols[i] = suffix_idx ? string_set.base_string()[ string_off + suffix_idx-1u ] : 255u;

            extract_word_packed<32u,4u,2u>(
                base_words,
                string_len,
                string_off,
                suffix_idx,
                word_begin,
                word_end,
                strided_iterator<uint32*>( h_radices + i, n_suffixes ) );
        }
    }
}

void extract_radices_64(
    const priv::string_set_2bit_u64_be  string_set,
    const uint32                        n_suffixes,
    const uint32                        word_begin,
    const uint32                        word_end,
    const uint2*                        h_suffixes,
          uint64*                       h_radices,
          uint8*                        h_symbols)
{
    if (word_begin+1 == word_end)
    {
        #pragma omp parallel for
        for (int32 i = 0; i < int32( n_suffixes ); ++i)
        {
            const uint2 suffix = h_suffixes[ i ];
            h_radices[i] = extract_radix_64( string_set, suffix, word_begin );
        }
    }
    else
    {
        #pragma omp parallel for
        for (int32 i = 0; i < int32( n_suffixes ); ++i)
        {
            const uint2 local_suffix_idx = h_suffixes[ i ];

            const uint32 string_idx = local_suffix_idx.y;
            const uint32 suffix_idx = local_suffix_idx.x;

            const uint64 string_off = string_set.offsets()[ string_idx ];
            const uint64 string_end = string_set.offsets()[ string_idx+1u ];
            const uint64 string_len = uint32( string_end - string_off );

            const uint64* base_words = string_set.base_string().container().stream();

            if (h_symbols != NULL)
                h_symbols[i] = suffix_idx ? string_set.base_string()[ string_off + suffix_idx - 1u ] : 255u;

            extract_word_packed<64u,5u,2u>(
                base_words,
                string_len,
                string_off,
                suffix_idx,
                word_begin,
                word_end,
                strided_iterator<uint64*>( h_radices + i, n_suffixes ) );
        }
    }
}

void extract_radices(
    const priv::string_set_2bit_be  string_set,
    const uint32                    n_suffixes,
    const uint32                    word_begin,
    const uint32                    word_end,
    const uint32                    word_bits,
    const uint2*                    h_suffixes,
          uint32*                   h_radices,
          uint8*                    h_symbols)
{
    if (word_bits == 16)
    {
        extract_radices_16(
            string_set,
            n_suffixes,
            word_begin,
            word_end,
            h_suffixes,
            h_radices,
            h_symbols );
    }
    else if (word_bits == 32)
    {
        extract_radices_32(
            string_set,
            n_suffixes,
            word_begin,
            word_end,
            h_suffixes,
            h_radices,
            h_symbols );
    }
    else
    {
        log_error(stderr,"extract_radices(): unsupported number of bits\n");
        exit(1);
    }
}

void extract_radices(
    const priv::string_set_2bit_u64_be  string_set,
    const uint32                        n_suffixes,
    const uint32                        word_begin,
    const uint32                        word_end,
    const uint32                        word_bits,
    const uint2*                        h_suffixes,
          uint64*                       h_radices,
          uint8*                        h_symbols)
{
    if (word_bits == 64)
    {
        extract_radices_64(
            string_set,
            n_suffixes,
            word_begin,
            word_end,
            h_suffixes,
            h_radices,
            h_symbols );
    }
    else
    {
        log_error(stderr,"extract_radices(): unsupported number of bits\n");
        exit(1);
    }
}

} // namespace priv
} // namespace nvbio
