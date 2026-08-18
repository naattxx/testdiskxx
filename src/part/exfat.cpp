/*

    File: exfat.c

    Copyright (C) 2009 Christophe GRENIER <grenier@cgsecurity.org>

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
// #include "types.h"
#include "exfat.hpp"
#include "src/common.hpp"
#include "src/log.hpp"

extern const arch_fnct_t arch_none;

auto exfat_cluster_to_offset(const struct exfat_super_block *exfat_header,
                             const unsigned int cluster) -> uint64_t
{
  return (static_cast<uint64_t>(((cluster - 2)
                                 << exfat_header->block_per_clus_bits) +
                                le32(exfat_header->clus_blocknr)))
      << exfat_header->blocksize_bits;
}

auto exfat_read_cluster(disk_t &disk, const partition_t &partition,
                        const struct exfat_super_block *exfat_header,
                        void *buffer, const unsigned int cluster) -> int
{
  return disk.pread(
      disk, buffer,
      1 << (exfat_header->block_per_clus_bits + exfat_header->blocksize_bits),
      partition.part_offset + exfat_cluster_to_offset(exfat_header, cluster)
  );
}

/*@
  @ requires \valid(partition);
  @ requires \valid_read(exfat_header);
  @ requires \separated(partition, exfat_header);
  @*/
static void set_exFAT_info(partition_t &partition,
                           const struct exfat_super_block *exfat_header)
{
  partition.upart_type = UP_EXFAT;
  partition.blocksize =
      1 << (exfat_header->block_per_clus_bits + exfat_header->blocksize_bits);
  partition.fsname[0] = '\0';
  if (partition.sb_offset == 0)
    snprintf(partition.info, sizeof(partition.info), "exFAT, blocksize=%u",
             partition.blocksize);
  else
    snprintf(partition.info, sizeof(partition.info),
             "exFAT found using backup sector, blocksize=%u",
             partition.blocksize);
}

auto check_exFAT(disk_t &disk, partition_t &partition) -> int
{
  auto *buffer = new unsigned char[EXFAT_BS_SIZE];
  if (disk.pread(disk, buffer, EXFAT_BS_SIZE, partition.part_offset) !=
      EXFAT_BS_SIZE)
  {
    delete[] (buffer);
    return 1;
  }
  if (test_exFAT(reinterpret_cast<struct exfat_super_block *>(buffer)) != 0)
  {
    delete[] (buffer);
    return 1;
  }
  set_exFAT_info(partition,
                 reinterpret_cast<struct exfat_super_block *>(buffer));
  delete[] (buffer);
  return 0;
}

auto test_exFAT(const struct exfat_super_block *exfat_header) -> int
{
  if (le16(exfat_header->signature) != 0xAA55)
    return 1;
  if (memcmp(exfat_header->oem_id, "EXFAT   ", sizeof(exfat_header->oem_id)) !=
      0)
    return 1;
  return 0;
}

auto recover_exFAT(const disk_t &disk,
                   const struct exfat_super_block *exfat_header,
                   partition_t &partition) -> int
{
  if (test_exFAT(exfat_header) != 0)
    return 1;
  partition.sborg_offset   = 0;
  partition.sb_size        = 12 << exfat_header->blocksize_bits;
  partition.part_type_i386 = P_EXFAT;
  partition.part_type_gpt  = GPT_ENT_TYPE_MS_BASIC_DATA;
  partition.part_size      = le64(exfat_header->nr_sectors) * disk.sector_size;
#ifdef DEBUG_exFAT
  log_info("recover_exFAT:\n");
  log_info("start_sector={}\n",
           (long long unsigned)le64(exfat_header->start_sector));
  log_info("blocksize={}\n", (12 << exfat_header->blocksize_bits));
  log_info("part_offset={}\n", partition.part_offset);
#endif
  if ((le64(exfat_header->start_sector) * disk.sector_size +
           (12 << exfat_header->blocksize_bits) ==
       partition.part_offset) ||
      (disk.arch == &arch_none &&
       (static_cast<uint64_t>(12) << exfat_header->blocksize_bits) ==
           partition.part_offset))
  {
    partition.sb_offset = 12 << exfat_header->blocksize_bits;
    partition.part_offset -= partition.sb_offset;
  }
  set_exFAT_info(partition, exfat_header);
  return 0;
}
