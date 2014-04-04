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

#pragma once

#include <nvbio/basic/types.h>
#include <nvbio/basic/numbers.h>
#include <nvbio/basic/packedstream.h>
#include <nvbio/basic/cuda/primitives.h>
#include <nvbio/basic/thrust_view.h>
#include <thrust/host_vector.h>
#include <thrust/device_vector.h>
#include <thrust/scatter.h>
#include <thrust/for_each.h>
#include <thrust/iterator/constant_iterator.h>

///\page qgroup_page Q-Group Module
///\htmlonly
/// <img src="nvidia_cubes.png" style="position:relative; bottom:-10px; border:0px;"/>
///\endhtmlonly
///\par
/// This module contains a series of functions to build a Q-Group Index, as described
/// in: \n
///
/// <i>Massively parallel read mapping on GPUs with PEANUT</i> \n
/// Johannes Koester and Sven Rahmann \n
/// http://arxiv.org/pdf/1403.1706v1.pdf
///
///
/// \section TechnicalOverviewSection Technical Overview
///\par
/// A complete list of the classes and functions in this module is given in the \ref QGroupIndex documentation.
///

namespace nvbio {

///
///@defgroup QGroupIndex QGroupIndex Module
/// This module contains a series of functions to build a Q-Group Index, as described
/// in: \n
///
/// <i>Massively parallel read mapping on GPUs with PEANUT</i> \n
/// Johannes Koester and Sven Rahmann \n
/// http://arxiv.org/pdf/1403.1706v1.pdf
///@{
///

/// A plain view of a q-group
///
struct QGroupIndexView
{
    static const uint32 WORD_SIZE = 32;

    typedef uint32*                                             vector_type;
    typedef PackedStream<const uint32*,uint8,1u,false,int64>    const_bitstream_type;
    typedef PackedStream<uint32*,uint8,1u,false,int64>          bitstream_type;

    /// constructor
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    QGroupIndexView(
        const uint32 _Q                 = 0,
        const uint32 _n_unique_qgrams   = 0,
        vector_type  _I                 = NULL,
        vector_type  _S                 = NULL,
        vector_type  _SS                = NULL,
        vector_type  _P                 = NULL) :
        Q               (_Q),
        n_unique_qgrams (_n_unique_qgrams),
        I               (_I),
        S               (_S),
        SS              (_SS),
        P               (_P)    {}

    /// return the slots of P corresponding to the given qgram g
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    uint2 slots(const uint64 g)
    {
        const uint32 i = uint32( g / WORD_SIZE );
        const uint32 j = uint32( g % WORD_SIZE );

        // compute j' such that bit j is the j'-th set bit in I[i]
        const uint32 j_prime = popc( I[i] & ((2u << j) - 1u) );

        return make_uint2(
            SS[ S[i] + j_prime ],
            SS[ S[i] + j_prime ] + 1u );
    }

    uint32        Q;
    uint32        n_unique_qgrams;
    vector_type   I;
    vector_type   S;
    vector_type   SS;
    vector_type   P;
};

/// A host-side q-group
///
struct QGroupIndexHost
{
    static const uint32 WORD_SIZE = 32;

    typedef thrust::host_vector<uint32>                         vector_type;
    typedef PackedStream<const uint32*,uint8,1u,false,int64>    const_bitstream_type;
    typedef PackedStream<uint32*,uint8,1u,false,int64>          bitstream_type;
    typedef QGroupIndexView                                     view_type;

    /// return the amount of device memory used
    ///
    uint64 used_host_memory() const
    {
        return I.size() * sizeof(uint32) +
               S.size() * sizeof(uint32) +
               SS.size() * sizeof(uint32) +
               P.size() * sizeof(uint32);
    }

    /// return the amount of device memory used
    ///
    uint64 used_device_memory() const { return 0u; }

    uint32        Q;
    uint32        n_unique_qgrams;
    vector_type   I;
    vector_type   S;
    vector_type   SS;
    vector_type   P;
};

/// A device-side q-group
///
struct QGroupIndexDevice
{
    static const uint32 WORD_SIZE = 32;

    typedef thrust::device_vector<uint32>                       vector_type;
    typedef PackedStream<const uint32*,uint8,1u,false,int64>    const_bitstream_type;
    typedef PackedStream<uint32*,uint8,1u,false,int64>          bitstream_type;
    typedef QGroupIndexView                                     view_type;

    /// build a q-group index from a given string T; the amount of storage required
    /// is basically O( A^q + |T|*32 ) bits, where A is the alphabet size.
    ///
    /// \tparam SYMBOL_SIZE     the size of the symbols, in bits
    /// \tparam string_type     the string iterator type
    ///
    /// \param q                the q parameter
    /// \param string_len       the size of the string
    /// \param string           the string iterator
    ///
    template <uint32 SYMBOL_SIZE, typename string_type>
    void build(
        const uint32        q,
        const uint32        string_len,
        const string_type   string);

    /// return the amount of device memory used
    ///
    uint64 used_host_memory() const { return 0u; }

    /// return the amount of device memory used
    ///
    uint64 used_device_memory() const
    {
        return I.size() * sizeof(uint32) +
               S.size() * sizeof(uint32) +
               SS.size() * sizeof(uint32) +
               P.size() * sizeof(uint32);
    }

    uint32        Q;
    uint32        n_unique_qgrams;
    vector_type   I;
    vector_type   S;
    vector_type   SS;
    vector_type   P;
};

/// return the plain view of a QGroupIndex
///
QGroupIndexView plain_view(QGroupIndexDevice& qgroup)
{
    return QGroupIndexView(
        qgroup.Q,
        qgroup.n_unique_qgrams,
        nvbio::plain_view( qgroup.I ),
        nvbio::plain_view( qgroup.S ),
        nvbio::plain_view( qgroup.SS ),
        nvbio::plain_view( qgroup.P ) );
}

/// A utility functor to extract the i-th q-gram out of a string
///
/// \tparam SYMBOL_SIZE         the size of the symbols, in bits
/// \tparam string_type         the string iterator type
///
template <uint32 SYMBOL_SIZE, typename string_type>
struct string_qgram_functor
{
    static const uint32 WORD_SIZE        = 32;
    static const uint32 SYMBOLS_PER_WORD = WORD_SIZE / SYMBOL_SIZE;

    /// constructor
    ///
    /// \param _string_len       string length
    /// \param _string           string iterator
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    string_qgram_functor(const uint32 _string_len, const string_type _string) :
        string_len(_string_len),
        string(_string) {}

    /// functor operator
    ///
    /// \param i        position along the string
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    uint64 operator() (const uint32 i) const
    {
        uint64 qgram = 0u;
        for (uint32 j = 0; j < SYMBOLS_PER_WORD; ++j)
            qgram |= uint64(i+j < string_len ? string[i + j] : 0u) << (j*SYMBOL_SIZE);

        return qgram;
    }

    const uint32        string_len; ///< string length
    const string_type   string;     ///< string iterator
};

///@} // end of the QGroupIndex group

} // namespace nvbio

#include <nvbio/qgroup/qgroup_inl.h>