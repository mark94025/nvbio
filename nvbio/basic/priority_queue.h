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

/*! \file priority_queue.h
 *   \brief A CUDA-compatible, fixed-size priority queue
 */

#pragma once

#include <nvbio/basic/types.h>

namespace nvbio {

/// \page priority_queues_page Priority Queues
///
/// This module implements a priority queue adaptor, supporting O(log(N)) push/pop operations.
/// Unlike std::priority_queue, this class can be used both in host and device CUDA code:
///
/// - priority_queue
///
/// \section ExampleSection Example
///
///\code
/// // build a simple priority_queue over 4 integers
/// typedef vector_wrapper<uint32*>             vector_type;
/// typedef priority_queue<uint32, vector_type> queue_type;
///
/// uint32 queue_storage[4];
///
/// // construct the queue
/// queue_type queue( vector_type( 0u, queue_storage ) );
///
/// // push a few items
/// queue.push( 3 );
/// queue.push( 8 );
/// queue.push( 1 );
/// queue.push( 5 );
///
/// // pop from the top
/// printf( "%u\n", queue.top() );      // -> 8
/// queue.pop();
/// printf( "%u\n", queue.top() );      // -> 5
/// queue.pop();
/// printf( "%u\n", queue.top() );      // -> 3
/// queue.pop();
/// printf( "%u\n", queue.top() );      // -> 1
///\endcode
///

///@addtogroup Basic
///@{

///@defgroup PriorityQueues Priority Queues
/// This module implements a priority queue adaptor.
///@{
    
///
/// A priority queue
///
/// \tparam Key         the key type
/// \tparam Container   the underlying container used to hold keys, must implement push_back(), size(), resize(), clear()
/// \tparam Compare     the comparison binary functor, Compare(a,b) == true iff a < b
///
template <typename Key, typename Container, typename Compare>
struct priority_queue
{
    /// constructor
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE priority_queue(Container cont = Container(), const Compare cmp = Compare());

    /// is queue empty?
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE bool  empty() const;

    /// return queue size
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE uint32 size() const;

    /// push an element
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE void push(const Key key);

    /// pop an element
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE void pop();

    /// top of the queue
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE Key top() const;

    /// return the i-th element in the heap
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE const Key& operator[] (const uint32 i) const;

    /// clear the queue
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE void clear();

    uint32      m_size;
    Container   m_queue;
    Compare     m_cmp;
};

///@} PriorityQueues
///@} Basic

} // namespace nvbio

#include <nvbio/basic/priority_queue_inline.h>
