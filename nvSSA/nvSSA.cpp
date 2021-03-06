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

// cuFMIndex.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <nvbio/basic/console.h>
#include <nvbio/io/fmi.h>

void crcInit();

using namespace nvbio;

int main(int argc, char* argv[])
{
    cudaSetDeviceFlags( cudaDeviceMapHost );

    crcInit();

    if (argc == 1)
    {
        log_info(stderr,"nvSSA [-gpu] input-prefix [output-prefix]\n");
        exit(0);
    }

    int base_arg = 0;
    const char* input;
    const char* output;
    if (strcmp( argv[1], "-gpu" ) == 0)
        base_arg = 2;
    else
        base_arg = 1;

    input = argv[base_arg];
    if (argc == base_arg+2)
        output = argv[base_arg+1];
    else
        output = argv[base_arg];

    //
    // Save sampled suffix array in a format compatible with BWA's
    //
    nvbio::io::FMIndexDataRAM driver_data;
    if (!driver_data.load( input ))
        return 1;

    nvbio::io::FMIndexData::SSA_type ssa, rssa;

    if (strcmp( argv[1], "-gpu" ) == 0)
    {
        nvbio::io::FMIndexDataDevice driver_data_cuda(
            driver_data,
            nvbio::io::FMIndexDataDevice::FORWARD | nvbio::io::FMIndexDataDevice::REVERSE );

        nvbio::io::FMIndexDataDevice::SSA_device_type ssa_cuda, rssa_cuda;

        init_ssa( driver_data_cuda, ssa_cuda, rssa_cuda );

        ssa  = ssa_cuda;
        rssa = rssa_cuda;
    }
    else
        init_ssa( driver_data, ssa, rssa );

    const uint32 sa_intv = nvbio::io::FMIndexData::SA_INT;
    const uint32 ssa_len = (driver_data.seq_length + sa_intv) / sa_intv;

    log_info(stderr, "saving SSA... started\n");
    {
        std::string file_name = std::string( output ) + std::string(".sa");
        FILE* file = fopen( file_name.c_str(), "wb" );

        fwrite( &driver_data.primary,       sizeof(uint32), 1u, file );
        fwrite( &driver_data.L2+1,          sizeof(uint32), 4u, file );
        fwrite( &sa_intv,                   sizeof(uint32), 1u, file );
        fwrite( &driver_data.seq_length,    sizeof(uint32), 1u, file );
        fwrite( &ssa.m_ssa[1],              sizeof(uint32), ssa_len-1, file );
        fclose( file );
    }
    {
        std::string file_name = std::string( output ) + std::string(".rsa");
        FILE* file = fopen( file_name.c_str(), "wb" );

        fwrite( &driver_data.rprimary,      sizeof(uint32), 1u, file );
        fwrite( &driver_data.rL2+1,         sizeof(uint32), 4u, file );
        fwrite( &sa_intv,                   sizeof(uint32), 1u, file );
        fwrite( &driver_data.seq_length,    sizeof(uint32), 1u, file );
        fwrite( &rssa.m_ssa[1],             sizeof(uint32), ssa_len-1, file );
        fclose( file );
    }
    log_info(stderr, "saving SSA... done\n");
    return 0;
}

