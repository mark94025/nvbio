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

#include <nvbio/basic/types.h>
#include <nvbio/basic/packedstream.h>
#include <nvbio/fmindex/rank_dictionary.h>

namespace nvbio {

///
/// \page fmindex_page FM-Index Module
///\htmlonly
/// <img src="nvidia_cubes.png" style="position:relative; bottom:-10px; border:0px;"/>
///\endhtmlonly
///
///\n
/// This module defines a series of classes to represent and operate on 2-bit FM-indices,
/// both from the host and device CUDA code.
///
/// \section RankDictionarySection Rank Dictionaries
///\par
/// A rank_dictionary is a data structure that, given a text T, allows to count the number
/// of occurrences of any given character c in a given prefix T[0,i] in O(1) time.
///\par
/// rank_dictionary is <i>storage-free</i>, in the sense it doesn't directly hold any allocated
/// data - hence it can be instantiated both on host and device data-structures, and it can
/// be conveniently passed as a kernel parameter (provided its template parameters are also
/// PODs).
///\par
/// It supports the following functions:
///\par
/// <table>
/// <tr><td style="white-space: nowrap; vertical-align:text-top;">
/// <b>Function</b>
/// </td><td style="vertical-align:text-top;">
/// <b>Inputs</b>
/// </td><td style="vertical-align:text-top;">
/// <b>Description</b>
/// </td></tr>
/// <tr><td style="white-space: nowrap; vertical-align:text-top;">
/// rank()<br>
/// </td><td style="vertical-align:text-top;">
/// dict, i, c
/// </td><td style="vertical-align:text-top;">
/// return the number of occurrences of a given character c in the prefix [0,i]
/// </td></tr>
/// <tr><td style="white-space: nowrap; vertical-align:text-top;">
/// rank()<br>
/// </td><td style="vertical-align:text-top;">
/// dict, range, c
/// </td><td style="vertical-align:text-top;">
/// return the number of occurrences of a given character c in the prefixes [0,range.x] and [0,range.y]
/// </td></tr>
/// <tr><td style="white-space: nowrap; vertical-align:text-top;">
/// rank4()<br>
/// </td><td style="vertical-align:text-top;">
/// dict, i
/// </td><td style="vertical-align:text-top;">
/// return the number of occurrences of all characters of a 4-letter alphabet in the prefix [0,i]
/// </td></tr>
/// <tr><td style="white-space: nowrap; vertical-align:text-top;">
/// rank4()<br>
/// </td><td style="vertical-align:text-top;">
/// dict, range
/// </td><td style="vertical-align:text-top;">
/// return the number of occurrences of all characters of a 4-letter alphabet in the prefixes [0,range.x] and [0,range.y]
/// </td></tr>
/// </table>
///
/// \section SSASection Sampled Suffix Arrays
///\par
/// A <i>Sampled Suffix Array</i> is a succint suffix array which has been sampled at a subset of
/// its indices. Ferragina & Manzini showed that given such a data structure and the BWT of the
/// original text it is possible to reconstruct the missing locations.
/// Two such data structures have been proposed, with different tradeoffs:
///  - one storing only the entries that are a multiple of K, { SA[i] | SA[i] % K = 0 }
///  - one storing only the entries whose index is a multiple of K, { SA[i] | i % K = 0 }
///\par
/// NVBIO provides both:
///\par
///  - SSA_value_multiple
///  - SSA_index_multiple
///\par
/// Unlike for the rank_dictionary, which is a storage-free class, these classes own the (internal) storage
/// needed to represent the underlying data structures, which resides on the host.
/// Similarly, the module provides some counterparts that hold the corresponding storage for the device:
///\par
///  - SSA_value_multiple_device
///  - SSA_index_multiple_device
///\par
/// While these classes hold device data, they are meant to be used from the host and cannot be directly
/// passed to CUDA kernels.
/// Plain views (see \ref host_device_page), or <i>contexts</i>, can be obtained with the usual plain_view() function.
///\par
/// The contexts expose the following interface:
/// \code
/// struct SSA
/// {
///     // return the i-th suffix array entry, if present
///     bool fetch(const uint32 i, uint32& r) const
///
///     // return whether the i-th suffix array is present
///     bool has(const uint32 i) const
/// }
/// \endcode
///\par
/// Detailed documentation can be found in the \ref SSAModule module documentation.
///
/// \section FMIndexSection FM-Indices
///\par
/// An fm_index is a self-compressed text index as described by Ferragina & Manzini.
/// It is built on top of the following ingredients:
///     - the BWT of a text T
///     - a rank dictionary (\ref rank_dictionary) of the given BWT
///     - a sampled suffix array of T
///\par
/// Given the above, it allows to count and locate all the occurrences in T of arbitrary patterns
/// P in O(length(P)) time.
/// Moreover, it does so with an incremental algorithm that proceeds character by character,
/// a fundamental property that allows to implement sophisticated pattern matching algorithms
/// based on backtracking.
///\par
/// fm_index is <i>storage-free</i>, in the sense it doesn't directly hold any allocated
/// data - hence it can be instantiated both on host and device data-structures, and it can
/// be conveniently passed as a kernel parameter (provided its template parameters are also
/// PODs).
///\par
/// It supports the following functions:
/// <table>
/// <tr><td style="white-space: nowrap; vertical-align:text-top;">
/// <b>Function</b>
/// </td><td style="vertical-align:text-top;">
/// <b>Inputs</b>
/// </td><td style="vertical-align:text-top;">
/// <b>Description</b>
/// </td></tr>
/// <tr><td style="white-space: nowrap; vertical-align:text-top;">
/// rank()<br>
/// </td><td style="vertical-align:text-top;">
/// fmi, i, c
/// </td><td style="vertical-align:text-top;">
/// return the number of occurrences of a given character c in the prefix [0,i]
/// </td></tr>
/// <tr><td style="white-space: nowrap; vertical-align:text-top;">
/// rank4()<br>
/// </td><td style="vertical-align:text-top;">
/// fmi, i
/// </td><td style="vertical-align:text-top;">
/// return the number of occurrences of all characters of a 4-letter alphabet in the prefix [0,i]
/// </td></tr>
/// <tr><td style="white-space: nowrap; vertical-align:text-top;">
/// match()<br>
/// </td><td style="vertical-align:text-top;">
/// fmi, pattern
/// </td><td style="vertical-align:text-top;">
/// return the SA range of occurrences of a given pattern
/// </td></tr>
/// <tr><td style="white-space: nowrap; vertical-align:text-top;">
/// match_reverse()<br>
/// </td><td style="vertical-align:text-top;">
/// fmi, pattern
/// </td><td style="vertical-align:text-top;">
/// return the SA range of occurrences of a given reversed pattern
/// </td></tr>
/// </td></tr>
/// <tr><td style="white-space: nowrap; vertical-align:text-top;">
/// locate()<br>
/// </td><td style="vertical-align:text-top;">
/// fmi, i
/// </td><td style="vertical-align:text-top;">
/// given a suffix array coordinate i, return its linear coordinate SA[i]
/// </td></tr>
/// </table>
///
/// \section FMIndexFiltersSection Batch Filtering
///\par
/// Performing massively parallel FM-index queries requires careful load balancing, as finding all
/// occurrences of a given set of strings in a text is a one-to-many process with variable-rate data expansion.
/// NVBIO offers simple host and device contexts to rank a set of strings and enumerate all their
/// occurrences automatically:
///\par
/// - \ref FMIndexFilterHost
/// - \ref FMIndexFilterDevice
///\par
/// Here is an example showing how to extract a set of seeds from a string set and find their occurrences
/// on the device:
///\code
/// template <typename fm_index_type>
/// void search_seeds(
///     const fm_index_type     fm_index,           // the input FM-index
///     const string_set_type   string_set,         // the input string-set
///     const uint32            seed_len,           // the seeds length
///     const uint32            seed_interval)      // the spacing between seeds
/// {
///     nvbio::vector<device_tag,string_set_infix_coord_type>&  seed_coords
///
///     // enumerate all seeds
///     const uint32 n_seeds = enumerate_string_set_seeds(
///         string_set,
///         uniform_seeds_functor<>( seed_len, seed_interval ),
///         seed_coords );
/// 
///     // and build the output infix-set
///     typedef InfixSet<string_set_type, const string_set_infix_coord_type*> seed_set_type;
///     seed_set_type seeds(
///         n_seeds,
///         string_set,
///         nvbio::plain_view( seed_coords ) );
///
///     // the filter
///     FMIndexFilterDevice fm_filter;
///
///     // first step: rank the query seeds
///     const uint64 n_hits = fm_filter.rank( fm_index, seeds );
/// 
///     // prepare storage for the output hits
///     nvbio::vector<device_tag,FMIndexFilterDevice::hit_type> hits( batch_size );
///
///     // loop through large batches of hits and locate & merge them
///     for (uint64 hits_begin = 0; hits_begin < n_hits; hits_begin += batch_size)
///     {
///         const uint64 hits_end = nvbio::min( hits_begin + batch_size, n_hits );
/// 
///         fm_filter.locate(
///             hits_begin,
///             hits_end,
///             hits.begin() );
///
///         // do something with the hits, e.g. extending them using DP alignment...
///         ...
///    }
/// }
///\endcode
///
/// \section MEMFiltersSection MEM Filtering
///\par
/// Additionally to simple filters above, NVBIO also some provides built-in functionality to find all (Super-) Maximal Extension Matches
/// in a string or a string-set:
///\par
///\par
/// - find_mems() : a host/device per-thread function to find all MEMs overlapping a given base of a pattern string
/// - \ref MEMFilterHost : a parallel host context to enumerate all MEMs of a string-set
/// - \ref MEMFilterDevice : a parallel device context to enumerate all MEMs of a string-set
///\par
/// The filters are analogous to the ones introduced in the previous section, except that rather than finding exact matches
/// for each string in a set, they will find all their MEMs or SMEMs.
///
/// \section PerformanceSection Performance
///\par
/// The graphs below show the performance of exact and approximate matching using the FM-index on the CPU and GPU,
/// searching for 32bp fragments inside the whole human genome. Approximate matching is performed with the hamming_backtrack()
/// function, in this case allowing up to 1 mismatch per fragment:
///
/// <img src="benchmark-fm-index.png" style="position:relative; bottom:-10px; border:0px;" width="55%" height="55%"/>
/// <img src="benchmark-fm-index-speedup.png" style="position:relative; bottom:-10px; border:0px;" width="55%" height="55%"/>
///
/// \section TechnicalOverviewSection Technical Overview
///\par
/// A complete list of the classes and functions in this module is given in the \ref FMIndex documentation.
///

///@addtogroup FMIndex
///@{

///\par
/// This class represents an FM-Index, i.e. the self-compressed text index by Ferragina & Manzini.
/// An FM-Index is built on top of the following ingredients:
///\par
///     - the BWT of a text T
///     - a rank dictionary (\ref rank_dictionary) of the given BWT
///     - a sampled suffix array of T
///\par
/// Given the above, it allows to count and locate all the occurrences in T of arbitrary patterns
/// P in O(length(P)) time.
/// Moreover, it does so with an incremental algorithm that proceeds character by character,
/// a fundamental property that allows to implement sophisticated pattern matching algorithms
/// based on backtracking.
///\par
/// fm_index is <i>storage-free</i>, in the sense it doesn't directly hold any allocated
/// data - hence it can be instantiated both on host and device data-structures, and it can
/// be conveniently passed as a kernel parameter (provided its template parameters are also
/// PODs)
///
/// \tparam TRankDictionary     a rank dictionary (see \ref RankDictionaryModule)
/// \tparam TSuffixArray        a sampled suffix array context implementing the \ref SSAInterface (see \ref SSAModule)
///
template <
    typename TRankDictionary,
    typename TSuffixArray>
struct fm_index
{
    typedef TRankDictionary                      rank_dictionary_type;
    typedef typename TRankDictionary::text_type  bwt_type;
    typedef TSuffixArray                         suffix_array_type;

    typedef typename TRankDictionary::index_type    index_type;
    typedef typename TRankDictionary::range_type    range_type;
    typedef typename TRankDictionary::vec4_type     vec4_type;

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE index_type      length() const { return m_length; }
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE index_type      primary() const { return m_primary; }
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE index_type      count(const uint32 c) const { return m_L2[c+1] - m_L2[c]; }
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE index_type      L2(const uint32 c) const { return m_L2[c]; }
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE TRankDictionary rank_dict() const { return m_rank_dict; }
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE TSuffixArray    sa() const { return m_sa; }
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE bwt_type        bwt() const { return m_rank_dict.text; }

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE fm_index() {}

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE fm_index(
        const index_type      length,
        const index_type      primary,
        const index_type*     L2,
        const TRankDictionary rank_dict,
        const TSuffixArray    sa) :
        m_length( length ),
        m_primary( primary ),
        m_L2( L2 ),
        m_rank_dict( rank_dict ),
        m_sa( sa )
    {}

    index_type          m_length;
    index_type          m_primary;
    const index_type*   m_L2;
    TRankDictionary     m_rank_dict;
    TSuffixArray        m_sa;
};

/// \relates fm_index
/// return the number of occurrences of c in the range [0,k] of the given FM-index.
///
/// \param fmi      FM-index
/// \param k        range search delimiter
/// \param c        query character
///
template <
    typename TRankDictionary,
    typename TSuffixArray>
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
typename fm_index<TRankDictionary,TSuffixArray>::index_type rank(
    const fm_index<TRankDictionary,TSuffixArray>&                   fmi,
    typename fm_index<TRankDictionary,TSuffixArray>::index_type     k,
    uint8                                                           c);

/// \relates fm_index
/// return the number of occurrences of c in the ranges [0,l] and [0,r] of the
/// given FM-index.
///
/// \param fmi      FM-index
/// \param range    range query [l,r]
/// \param c        query character
///
template <
    typename TRankDictionary,
    typename TSuffixArray>
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
typename fm_index<TRankDictionary,TSuffixArray>::range_type rank(
    const fm_index<TRankDictionary,TSuffixArray>&                   fmi,
    typename fm_index<TRankDictionary,TSuffixArray>::range_type     range,
    uint8                                                           c);

/// \relates fm_index
/// return the number of occurrences of all characters in the range [0,k] of the
/// given FM-index.
///
/// \param fmi      FM-index
/// \param k        range search delimiter
///
template <
    typename TRankDictionary,
    typename TSuffixArray>
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
typename fm_index<TRankDictionary,TSuffixArray>::vec4_type rank4(
    const fm_index<TRankDictionary,TSuffixArray>&                   fmi,
    typename fm_index<TRankDictionary,TSuffixArray>::index_type     k);

/// \relates fm_index
/// return the number of occurrences of all characters in the range [0,k] of the
/// given FM-index.
///
/// \param fmi      FM-index
/// \param range    range query [l,r]
/// \param outl     first output
/// \param outh     second output
///
template <
    typename TRankDictionary,
    typename TSuffixArray>
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE void rank4(
    const fm_index<TRankDictionary,TSuffixArray>&                   fmi,
    typename fm_index<TRankDictionary,TSuffixArray>::range_type     range,
    typename fm_index<TRankDictionary,TSuffixArray>::vec4_type*     outl,
    typename fm_index<TRankDictionary,TSuffixArray>::vec4_type*     outh);

/// \relates fm_index
/// return the range of occurrences of a pattern in the given FM-index.
///
/// \param fmi          FM-index
/// \param pattern      query string
/// \param pattern_len  query string length
///
template <
    typename TRankDictionary,
    typename TSuffixArray,
    typename Iterator>
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
typename fm_index<TRankDictionary,TSuffixArray>::range_type match(
    const fm_index<TRankDictionary,TSuffixArray>&                       fmi,
    const Iterator                                                      pattern,
    const uint32                                                        pattern_len);

/// \relates fm_index
/// return the range of occurrences of a pattern in the given FM-index.
///
/// \param fmi          FM-index
/// \param pattern      query string
/// \param pattern_len  query string length
/// \param range        start range
///
template <
    typename TRankDictionary,
    typename TSuffixArray,
    typename Iterator>
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
typename fm_index<TRankDictionary,TSuffixArray>::range_type match(
    const fm_index<TRankDictionary,TSuffixArray>&                       fmi,
    const Iterator                                                      pattern,
    const uint32                                                        pattern_len,
    const typename fm_index<TRankDictionary,TSuffixArray>::range_type   range);

/// \relates fm_index
/// return the range of occurrences of a reversed pattern in the given FM-index.
///
/// \param fmi          FM-index
/// \param pattern      query string
/// \param pattern_len  query string length
///
template <
    typename TRankDictionary,
    typename TSuffixArray,
    typename Iterator>
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
typename fm_index<TRankDictionary,TSuffixArray>::range_type match_reverse(
    const fm_index<TRankDictionary,TSuffixArray>&                       fmi,
    const Iterator                                                      pattern,
    const uint32                                                        pattern_len);

// \relates fm_index
// computes the inverse psi function at a given index, without using the reduced SA
//
// \param fmi          FM-index
// \param i            query index
// \return             base inverse psi function value and offset
//
template <
    typename TRankDictionary,
    typename TSuffixArray>
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
typename fm_index<TRankDictionary,TSuffixArray>::index_type basic_inv_psi(
    const fm_index<TRankDictionary,TSuffixArray>&                       fmi,
    const typename fm_index<TRankDictionary,TSuffixArray>::range_type   i);

/// \relates fm_index
/// computes the inverse psi function at a given index
///
/// \param fmi          FM-index
/// \param i            query index
/// \return             base inverse psi function value and offset
///
template <
    typename TRankDictionary,
    typename TSuffixArray>
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
typename fm_index<TRankDictionary,TSuffixArray>::range_type inv_psi(
    const fm_index<TRankDictionary,TSuffixArray>&                       fmi,
    const typename fm_index<TRankDictionary,TSuffixArray>::index_type   i);

/// \relates fm_index
/// return the linear coordinate of the suffix that prefixes the i-th row of the BWT matrix.
///
/// \param fmi          FM-index
/// \param i            query index
/// \return             position of the suffix that prefixes the query index
///
template <
    typename TRankDictionary,
    typename TSuffixArray>
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
typename fm_index<TRankDictionary,TSuffixArray>::index_type locate(
    const fm_index<TRankDictionary,TSuffixArray>&                       fmi,
    const typename fm_index<TRankDictionary,TSuffixArray>::index_type   i);

/// \relates fm_index
/// return the position of the suffix that prefixes the i-th row of the BWT matrix in the sampled SA,
/// and its relative offset
///
/// \param fmi          FM-index
/// \param i            query index
/// \return             a pair formed by the position of the suffix that prefixes the query index
///                     in the sampled SA and its relative offset
///
template <
    typename TRankDictionary,
    typename TSuffixArray>
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
typename fm_index<TRankDictionary,TSuffixArray>::range_type locate_ssa_iterator(
    const fm_index<TRankDictionary,TSuffixArray>&                       fmi,
    const typename fm_index<TRankDictionary,TSuffixArray>::index_type   i);

/// \relates fm_index
/// return the position of the suffix that prefixes the i-th row of the BWT matrix.
///
/// \param fmi          FM-index
/// \param iter         iterator to the sampled SA
/// \return             final linear coordinate
///
template <
    typename TRankDictionary,
    typename TSuffixArray>
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
typename fm_index<TRankDictionary,TSuffixArray>::index_type lookup_ssa_iterator(
    const fm_index<TRankDictionary,TSuffixArray>&                       fmi,
    const typename fm_index<TRankDictionary,TSuffixArray>::range_type   it);

#ifdef __CUDACC__
#if defined(MOD_NAMESPACE)
MOD_NAMESPACE_BEGIN
#endif

#if USE_TEX
texture<uint32> s_count_table_tex;
#endif
///
/// Helper structure for handling the (global) count table texture
///
struct count_table_texture
{
    /// indexing operator
    ///
    NVBIO_FORCEINLINE NVBIO_DEVICE uint32 operator[] (const uint32 i) const;

    /// bind texture
    ///
    NVBIO_FORCEINLINE NVBIO_HOST static void bind(const uint32* count_table);

    /// unbind texture
    ///
    NVBIO_FORCEINLINE NVBIO_HOST static void unbind();
};

#if defined(MOD_NAMESPACE)
MOD_NAMESPACE_END
#endif
#endif

///@} FMIndex

} // namespace nvbio

#include <nvbio/fmindex/fmindex_inl.h>
