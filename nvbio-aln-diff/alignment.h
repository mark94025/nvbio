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
#include <nvbio/basic/numbers.h>

namespace nvbio {
namespace alndiff {

struct Alignment
{
    enum { PAIRED        = 1,
           PROPER_PAIR   = 2,
           UNMAPPED      = 4,
           MATE_UNMAPPED = 8,
           REVERSE       = 16,
           MATE_REVERSE  = 32,
           READ_1        = 64,
           READ_2        = 128,
           SECONDARY     = 256,
           QC_FAILED     = 512,
           DUPLICATE     = 1024
         };

    Alignment() :
        read_id( uint32(-1) ),
        read_len( 0 ),
        mate( 0 ),
        pos( 0 ),
        ref_id( uint32(-1) ),
        flag( UNMAPPED ),
        score( -65536 ),
        mapQ( 0 ),
        ed( 255 ),
        subs( 0 ),
        ins( 0 ),
        dels( 0 ),
        n_mm( 0 ),
        n_gapo( 0 ),
        n_gape( 0 ),
        has_second( 0 ),
        sec_score( -65536 ) {}

    bool is_mapped()    const { return (pos != 0) && ((flag & UNMAPPED) == 0); }
    bool is_rc()        const { return (flag & REVERSE) != 0; }
    bool is_unique()    const { return is_mapped() && (has_second == false); }
    bool is_ambiguous() const { return is_mapped() && has_second && (sec_score == score); }

    uint16 mapped_read_bases() const { return subs + ins; }
    uint16 mapped_ref_bases()  const { return subs + dels; }
    uint16 trimmed()           const { return read_len - subs - ins; }

    uint32 read_id;
    uint32 read_len;
    uint32 mate;
    uint32 pos;
    uint32 ref_id;
    uint32 flag;
     int32 score;
    uint8  mapQ;
    uint8  ed;
    uint16 subs;
    uint16 ins;
    uint16 dels;
    uint8  n_mm;
    uint8  n_gapo;
    uint8  n_gape;
    uint8  has_second;
    int32  sec_score;
};

inline
bool distant(const Alignment& a1, const Alignment& a2)
{
    return (int64(a1.pos) < int64(a2.pos) - a1.read_len) ||
           (int64(a1.pos) > int64(a2.pos) + a1.read_len);
}

struct AlignmentPair
{
    AlignmentPair() {}
    AlignmentPair(const Alignment m1, const Alignment m2) : mate1(m1), mate2(m2) {}

    uint32 read_id()   const { return mate1.read_id; }
    uint32 read_len()  const { return mate1.read_len + mate2.read_len; }
     int32 score()     const { return mate1.score    + mate2.score; }
     uint8 mapQ()      const { return mate1.mapQ; }
     uint8 ed()        const { return mate1.ed       + mate2.ed; }
    uint16 subs()      const { return mate1.subs     + mate2.subs; }
    uint16 ins()       const { return mate1.ins      + mate2.ins; }
    uint16 dels()      const { return mate1.dels     + mate2.dels; }
     uint8 n_mm()      const { return mate1.n_mm     + mate2.n_mm; }
     uint8 n_gapo()    const { return mate1.n_gapo   + mate2.n_gapo; }
     uint8 n_gape()    const { return mate1.n_gape   + mate2.n_gape; }
     bool has_second() const { return mate1.has_second && mate2.has_second; }
     int32 sec_score() const { return mate1.sec_score  +  mate2.sec_score; }

    bool is_mapped()          const { return mate1.is_mapped() && mate2.is_mapped(); }
    bool is_mapped_paired()   const { return mate1.is_mapped() && mate2.is_mapped() && (mate1.flag & Alignment::PROPER_PAIR) && (mate2.flag & Alignment::PROPER_PAIR); }
    bool is_mapped_unpaired() const { return mate1.is_mapped() && mate2.is_mapped() && (((mate1.flag & Alignment::PROPER_PAIR) == 0) || ((mate2.flag & Alignment::PROPER_PAIR) == 0)); }
    bool is_unique_paired()   const { return is_mapped_paired() && mate1.is_unique() && mate2.is_unique(); }
    bool is_ambiguous_paired()const { return is_mapped_paired() && has_second() && (score() == sec_score()); }

    uint16 mapped_read_bases() const { return subs() + ins(); }
    uint16 mapped_ref_bases()  const { return subs() + dels(); }
    uint16 trimmed()           const { return read_len() - subs() - ins(); }

    const Alignment& operator[] (const uint32 i) const { return *(&mate1 + i); }

    Alignment mate1;
    Alignment mate2;
};

struct AlignmentStream
{
    /// virtual destructor
    ///
    virtual ~AlignmentStream() {}

    /// return if the stream is ok
    ///
    virtual bool is_ok() { return false; }

    /// get the next batch
    ///
    virtual uint32 next_batch(
        const uint32    count,
        Alignment*      batch) { return 0; }
};

/// open an alignment file
///
AlignmentStream* open_alignment_file(const char* file_name);

} // alndiff namespace
} // nvbio namespace
