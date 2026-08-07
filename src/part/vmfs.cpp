/*

    File: vmfs.c

    Copyright (C) 2011 Christophe GRENIER <grenier@cgsecurity.org>

    This software is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write the Free Software Foundation, Inc., 51
    Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

 */
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include "types.h"
#include "src/common.hpp"
#include "src/fnctdsk.hpp"
#include "src/log.hpp"
#include "vmfs.hpp"

static void set_VMFS_info(const struct vmfs_volume *sb, partition_t &partition)
{
    partition.upart_type = UP_VMFS;
    sprintf(partition.info, "VMFS %lu", (long unsigned)le32(sb->version));
}

static int test_VMFS(const disk_t &disk, const struct vmfs_volume *sb, const partition_t &partition, const int dump_ind)
{
    if (le32(sb->magic) != 0xc001d00d || le32(sb->version) > 20)
        return 1;
    if (dump_ind != 0)
    {
        log_info("\nVMFS magic value at {}/{}/{}\n", offset2cylinder(disk, partition.part_offset),
                    offset2head(disk, partition.part_offset), offset2sector(disk, partition.part_offset));
        ; // dump_log(sb,DEFAULT_SECTOR_SIZE);
    }
    return 0;
}
int check_VMFS(disk_t &disk, partition_t &partition)
{
    unsigned char *buffer = new unsigned char[2 * DEFAULT_SECTOR_SIZE];
    if (disk.pread(disk, buffer, 2 * DEFAULT_SECTOR_SIZE, partition.part_offset + 0x100000) != DEFAULT_SECTOR_SIZE)
    {
        delete[] (buffer);
        return 1;
    }
    if (test_VMFS(disk, (struct vmfs_volume *)buffer, partition, 0) != 0)
    {
        delete[] (buffer);
        return 1;
    }
    set_VMFS_info((struct vmfs_volume *)buffer, partition);
    delete[] (buffer);
    return 0;
}

int recover_VMFS(const disk_t &disk, const struct vmfs_volume *sb, partition_t &partition, const int verbose,
                 const int dump_ind)
{
    const struct vmfs_lvm *lvm = (const struct vmfs_lvm *)(((const char *)sb) + 0x200);
    if (test_VMFS(disk, sb, partition, dump_ind) != 0)
        return 1;
    set_VMFS_info(sb, partition);
    partition.part_type_i386 = P_VMFS;
    partition.part_size = (uint64_t)le64(lvm->size);
    partition.blocksize = 0;
    partition.sborg_offset = 0;
    partition.sb_offset = 0;
    if (verbose > 0)
    {
        log_info("\n");
    }
    return 0;
}
