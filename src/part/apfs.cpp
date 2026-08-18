/*

    File: apfs.c

    Copyright (C) 2021 Christophe GRENIER <grenier@cgsecurity.org>

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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
// #include "types.h"
#include "apfs.hpp"
#include "src/common.hpp"
#include "src/fnctdsk.hpp"
#include "src/guid_cpy.hpp"
#include "src/log.hpp"

static void set_APFS_info(const nx_superblock_t *sb, partition_t &partition)
{
  partition.upart_type = UP_APFS;
}

auto check_APFS(disk_t &disk_car, partition_t &partition) -> int
{
  auto *buffer   = new unsigned char[APFS_SUPERBLOCK_SIZE];
  const auto *sb = reinterpret_cast<const nx_superblock_t *>(buffer);
  if (disk_car.pread(disk_car, buffer, APFS_SUPERBLOCK_SIZE,
                     partition.part_offset) != APFS_SUPERBLOCK_SIZE)
  {
    delete[] (buffer);
    return 1;
  }
  if (test_APFS(sb, partition) != 0)
  {
    delete[] (buffer);
    return 1;
  }
  set_APFS_info(sb, partition);
  delete[] (buffer);
  return 0;
}

auto recover_APFS(const disk_t &disk, const nx_superblock_t *sb,
                  partition_t &partition, const int verbose, const int dump_ind)
    -> int
{
  if (test_APFS(sb, partition) != 0)
    return 1;
  if (dump_ind != 0)
  {
    log_info("\nAPFS magic value at {}/{}/{}",
             offset2cylinder(disk, partition.part_offset),
             offset2head(disk, partition.part_offset),
             offset2sector(disk, partition.part_offset));
    /* There is a little offset ... */
    ; // dump_log(sb,DEFAULT_SECTOR_SIZE);
  }
  set_APFS_info(sb, partition);
  partition.part_type_i386 = P_LINUX;
  partition.part_type_mac  = PMAC_LINUX;
  partition.part_type_sun  = PSUN_LINUX;
  partition.part_type_gpt  = GPT_ENT_TYPE_MAC_APFS;
  partition.part_size      = le32(sb->nx_block_size) * le64(sb->nx_block_count);
  guid_cpy(&partition.part_uuid,
           reinterpret_cast<const efi_guid_t *>(&sb->nx_uuid));
  if (verbose > 0)
  {
    log_info("\n");
  }
  partition.sborg_offset = 0;
  partition.sb_size      = le32(sb->nx_block_size);
  partition.sb_offset    = 0;
  if (verbose > 0)
  {
    log_info("recover_APFS: s_blocksize={}", partition.blocksize);
    log_info("recover_APFS: s_blocks_count {}",
             (long unsigned int)le64(sb->nx_block_count));
    // if (disk == NULL)
    //     log_info("recover_APFS: part_size {}\n", (long
    //     unsigned)(partition.part_size / DEFAULT_SECTOR_SIZE));
    // else
    log_info("recover_APFS: part_size {}\n",
             (long unsigned)(partition.part_size / disk.sector_size));
  }
  return 0;
}
