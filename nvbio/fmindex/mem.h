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

#include <nvbio/fmindex/fmindex.h>
#include <nvbio/basic/types.h>
#include <nvbio/basic/numbers.h>
#include <nvbio/basic/algorithms.h>
#include <nvbio/basic/exceptions.h>
#include <nvbio/basic/vector.h>
#include <nvbio/basic/vector_array.h>
#include <nvbio/basic/cuda/sort.h>
#include <nvbio/basic/cuda/primitives.h>
#include <nvbio/strings/string.h>
#include <thrust/sort.h>
#include <thrust/scan.h>
#include <thrust/iterator/constant_iterator.h>
#include <thrust/iterator/counting_iterator.h>

namespace nvbio {

///@addtogroup FMIndex
///@{

/// find all MEMs (Maximal Extension Matches) covering a given base of a pattern
///
/// \tparam pattern_type        the pattern string type
/// \tparam delegate_type       the delegate output handler, must implement the following interface:
///\anchor MEMHandler
///\code
/// interface MEMHandler
/// {
///     typedef typename fm_index_type::range_type range_type;
///
///     // output an FM-index range referring to the forward index,
///     // together with its corresponding span on the pattern
///     void output(
///         const range_type    range,     // output SA range
///         const uint2         span);     // output pattern span
/// };
///\endcode
///
/// \param pattern_len          the length of the query pattern
/// \param pattern              the query pattern
/// \param x                    the base of the query pattern to cover with MEMs
/// \param f_index              the forward FM-index to match against
/// \param r_index              the reverse FM-index to match against
/// \param handler              the output handler
/// \param min_intv             the minimum SA interval size
/// \param min_span             the minimum pattern span size
///
/// \return the right-most end of the MEMs covering x, or x itself if no MEM was found
///
template <typename pattern_type, typename fm_index_type, typename delegate_type>
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
uint32 find_mems(
    const uint32            pattern_len,
    const pattern_type      pattern,
    const uint32            x,
    const fm_index_type     f_index,
    const fm_index_type     r_index,
          delegate_type&    handler,
    const uint32            min_intv = 1u,
    const uint32            min_span = 1u);

///
///\par
/// This class implements an FM-index filter which can be used to find and filter MEMs
/// between an arbitrary string-set and an \ref FMIndex "FM-index".
///\par
/// The filter is designed to:
///\par
///  - first <i>find and rank</i> the suffix array ranges containing occurrences of all MEMs,
/// expressed as a series of lists of <i>(SA-begin,SA-end,string-begin,string-end)</i> tuples,
/// one for each string in the set.
///  - and then <i>enumerate</i> each individual occurrence of a MEM within the lists,
///  as a set of <i>(index-pos,string-id,string-begin,string-end)</i> tuples.
///\par
/// \tparam fm_index_type    the type of the fm-index
///
template <typename system_tag, typename fm_index_type>
struct MEMFilter {};

///
///\par
/// This class implements an FM-index filter which can be used to find and filter MEMs
/// between an arbitrary string-set and an \ref FMIndex "FM-index".
///\par
/// The filter is designed to:
///\par
///  - first <i>find and rank</i> the suffix array ranges containing occurrences of all MEMs,
/// expressed as a series of lists of <i>(SA-begin, SA-end,string-begin,string-end)</i> tuples,
/// one for each string in the set.
///  - and then <i>enumerate</i> each individual occurrence of a MEM within the lists,
///  as a set of <i>(index-pos,string-id,string-begin,string-end)</i> tuples.
///\par
/// \tparam fm_index_type    the type of the fm-index
///
template <typename fm_index_type>
struct MEMFilter<host_tag, fm_index_type>
{
    typedef host_tag                                        system_tag;     ///< the backend system
    typedef fm_index_type                                   index_type;     ///< the index type

    typedef typename index_type::index_type                 coord_type;     ///< the coordinate type of the fm-index, uint32|uint64
    static const uint32                                     coord_dim = vector_traits<coord_type>::DIM;

    typedef typename vector_type<coord_type,4u>::type       rank_type;      ///< rank coordinates are either uint32_4 or uint64_4
    typedef typename vector_type<coord_type,4u>::type       mem_type;       ///< MEM coordinates are either uint32_4 or uint64_4
    typedef mem_type                                        hit_type;       ///< MEM coordinates are either uint32_4 or uint64_4

    /// enact the filter on an FM-index and a string-set
    ///
    /// \param index            the FM-index
    /// \param string-set       the query string-set
    ///
    /// \return the total number of mems
    ///
    template <typename string_set_type>
    uint64 rank(
        const fm_index_type&    f_index,
        const fm_index_type&    r_index,
        const string_set_type&  string_set,
        const uint32            min_intv = 1u);

    /// enumerate all mems in a given range
    ///
    /// \tparam mems_iterator         a mem_type iterator
    ///
    /// \param begin                  the beginning of the mems sequence to locate, in [0,n_mems)
    /// \param end                    the end of the mems sequence to locate, in [0,n_mems]
    ///
    template <typename mems_iterator>
    void locate(
        const uint64    begin,
        const uint64    end,
        mems_iterator   mems);

    /// return the number of mems from the last rank query
    ///
    uint64 n_hits() const { return m_n_occurrences; }

    /// return the number of mems from the last rank query
    ///
    uint64 n_mems() const { return m_n_occurrences; }

    /// return the number of MEM ranges
    ///
    uint64 n_ranges() const { return m_mem_ranges.allocated_size(); }

    uint32                              m_n_queries;
    index_type                          m_f_index;
    index_type                          m_r_index;
    uint64                              m_n_occurrences;
    HostVectorArray<rank_type>          m_mem_ranges;
    thrust::host_vector<uint64>         m_slots;
};

///
///\par
/// This class implements an FM-index filter which can be used to find and filter MEMs
/// between an arbitrary string-set and an \ref FMIndex "FM-index".
///\par
/// The filter is designed to:
///\par
///  - first <i>find and rank</i> the suffix array ranges containing occurrences of all MEMs,
/// expressed as a series of lists of <i>(SA-begin, SA-end,string-begin,string-end)</i> tuples,
/// one for each string in the set.
///  - and then <i>enumerate</i> each individual occurrence of a MEM within the lists,
///  as a set of <i>(index-pos,string-id,string-begin,string-end)</i> tuples.
///\par
/// \tparam fm_index_type    the type of the fm-index
///
template <typename fm_index_type>
struct MEMFilter<device_tag, fm_index_type>
{
    typedef device_tag                                      system_tag;     ///< the backend system
    typedef fm_index_type                                   index_type;     ///< the index type

    typedef typename index_type::index_type                 coord_type;     ///< the coordinate type of the fm-index, uint32|uint64
    static const uint32                                     coord_dim = vector_traits<coord_type>::DIM;

    typedef typename vector_type<coord_type,4u>::type       rank_type;      ///< rank coordinates are either uint32_4 or uint64_4
    typedef typename vector_type<coord_type,4u>::type       mem_type;       ///< MEM coordinates are either uint32_4 or uint64_4
    typedef mem_type                                        hit_type;       ///< MEM coordinates are either uint32_4 or uint64_4

    /// enact the filter on an FM-index and a string-set
    ///
    /// \param index            the FM-index
    /// \param string-set       the query string-set
    ///
    /// \return the total number of mems
    ///
    template <typename string_set_type>
    uint64 rank(
        const fm_index_type&    f_index,
        const fm_index_type&    r_index,
        const string_set_type&  string_set,
        const uint32            min_intv = 1u);

    /// enumerate all mems in a given range
    ///
    /// \tparam mems_iterator         a mem_type iterator
    ///
    /// \param begin                  the beginning of the mems sequence to locate, in [0,n_mems)
    /// \param end                    the end of the mems sequence to locate, in [0,n_mems]
    ///
    template <typename mems_iterator>
    void locate(
        const uint64    begin,
        const uint64    end,
        mems_iterator   mems);

    /// return the number of mems from the last rank query
    ///
    uint64 n_hits() const { return m_n_occurrences; }

    /// return the number of mems from the last rank query
    ///
    uint64 n_mems() const { return m_n_occurrences; }

    /// return the number of MEM ranges
    ///
    uint64 n_ranges() const { return m_mem_ranges.allocated_size(); }

    uint32                              m_n_queries;
    index_type                          m_f_index;
    index_type                          m_r_index;
    uint64                              m_n_occurrences;
    DeviceVectorArray<rank_type>        m_mem_ranges;
    thrust::device_vector<uint64>       m_slots;
    thrust::device_vector<uint8>        d_temp_storage;
};

///
///\par
/// This class implements an FM-index filter which can be used to find and filter MEMs
/// between an arbitrary string-set and an \ref FMIndex "FM-index".
///\par
/// The filter is designed to:
///\par
///  - first <i>find and rank</i> the suffix array ranges containing occurrences of all MEMs,
/// expressed as a series of lists of <i>(SA-begin, SA-end,string-begin,string-end)</i> tuples,
/// one for each string in the set.
///  - and then <i>enumerate</i> each individual occurrence of a MEM within the lists,
///  as a set of <i>(index-pos,string-id,string-begin,string-end)</i> tuples.
///\par
/// \tparam fm_index_type    the type of the fm-index
///
template <typename fm_index_type>
struct MEMFilterHost : public MEMFilter<host_tag, fm_index_type> {};

///
///\par
/// This class implements an FM-index filter which can be used to find and filter MEMs
/// between an arbitrary string-set and an \ref FMIndex "FM-index".
///\par
/// The filter is designed to:
///\par
///  - first <i>find and rank</i> the suffix array ranges containing occurrences of all MEMs,
/// expressed as a series of lists of <i>(SA-begin, SA-end,string-begin,string-end)</i> tuples,
/// one for each string in the set.
///  - and then <i>enumerate</i> each individual occurrence of a MEM within the lists,
///  as a set of <i>(index-pos,string-id,string-begin,string-end)</i> tuples.
///\par
/// \tparam fm_index_type    the type of the fm-index
///
template <typename fm_index_type>
struct MEMFilterDevice : public MEMFilter<device_tag, fm_index_type> {};

///@} // end of the FMIndex group

} // namespace nvbio

#include <nvbio/fmindex/mem_inl.h>
