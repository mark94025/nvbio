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

#include <nvbio/io/output/output_batch.h>
#include <nvbio/io/output/output_sam.h>
#include <nvbio/io/output/output_bam.h>
#include <nvbio/io/output/output_debug.h>

namespace nvbio {
namespace io {

OutputFile::OutputFile(const char *_file_name, AlignmentType _alignment_type, BNT _bnt)
    : file_name(_file_name),
      alignment_type(_alignment_type),
      bnt(_bnt),
      mapq_evaluator(NULL),
      mapq_filter(-1),
      read_data_1(NULL),
      read_data_2(NULL)
{
}

OutputFile::~OutputFile()
{
}

void OutputFile::configure_mapq_evaluator(const io::MapQEvaluator *mapq, int mapq_filter)
{
    OutputFile::mapq_evaluator = mapq;
    OutputFile::mapq_filter = mapq_filter;
}

void OutputFile::start_batch(const io::ReadData *read_data_1,
                             const io::ReadData *read_data_2)
{
    // stash the current host pointer for the read data
    OutputFile::read_data_1 = read_data_1;
    OutputFile::read_data_2 = read_data_2;
}

void OutputFile::process(struct GPUOutputBatch& gpu_batch,
                         const AlignmentMate alignment_mate,
                         const AlignmentScore alignment_score)
{
    // do nothing
}

void OutputFile::end_batch(void)
{
    // invalidate the read data pointers
    read_data_1 = NULL;
    read_data_2 = NULL;
}

void OutputFile::close(void)
{
}

IOStats& OutputFile::get_aggregate_statistics(void)
{
    return iostats;
}

void OutputFile::readback(struct CPUOutputBatch& cpu_batch,
                          const struct GPUOutputBatch& gpu_batch,
                          const AlignmentMate mate,
                          const AlignmentScore score)
{
    Timer timer;
    timer.start();

    // read back the scores
    // readback_scores will do the "right thing" based on the pass and score that we're getting
    // (i.e., it'll update either the first or second score for either the anchor or opposite mate)
    gpu_batch.readback_scores(cpu_batch.best_alignments, mate, score);

    if (score == BEST_SCORE)
    {
        // if this is the best alignment, read back CIGARs and MD strings as well
        gpu_batch.readback_cigars(cpu_batch.cigar[mate]);
        gpu_batch.readback_mds(cpu_batch.mds[mate]);

        if (mate == MATE_1)
        {
            // mate 1 best score comes first; stash the count
            cpu_batch.count = gpu_batch.count;

            // set up the read data pointers
            // this is not strictly related to which mate or scoring pass we're processing,
            // but must be done once per batch, so we do it here
            cpu_batch.read_data[MATE_1] = read_data_1;
            cpu_batch.read_data[MATE_2] = read_data_2;
        }
    }

    // sanity check to make sure the number of reads matches what we got previously
    // (for mate 1 best score this will always pass due to the assignment above)
    NVBIO_CUDA_ASSERT(cpu_batch.count == gpu_batch.count);

    timer.stop();
    iostats.output_process_timings.add(gpu_batch.count, timer.seconds());

    iostats.alignments_DtoH_time += timer.seconds();
    iostats.alignments_DtoH_count += gpu_batch.count;
}

OutputFile *OutputFile::open(const char *file_name, AlignmentType aln_type, BNT bnt)
{
    // parse out file extension; look for .sam, .bam suffixes
    uint32 len = uint32(strlen(file_name));

    if (strcmp(file_name, "/dev/null") == 0)
    {
        return new OutputFile(file_name, aln_type, bnt);
    }

    if (len >= strlen(".sam"))
    {
        if (strcmp(&file_name[len - strlen(".sam")], ".sam") == 0)
        {
            return new SamOutput(file_name, aln_type, bnt);
        }
    }

    if (len >= strlen(".bam"))
    {
        if (strcmp(&file_name[len - strlen(".bam")], ".bam") == 0)
        {
            return new BamOutput(file_name, aln_type, bnt);
        }
    }

    if (len >= strlen(".dbg"))
    {
        if (strcmp(&file_name[len - strlen(".dbg")], ".dbg") == 0)
        {
            return new DebugOutput(file_name, aln_type, bnt);
        }
    }

    log_warning(stderr, "could not determine file type for %s; guessing SAM\n", file_name);
    return new SamOutput(file_name, aln_type, bnt);
}

} // namespace io
} // namespace nvbio
