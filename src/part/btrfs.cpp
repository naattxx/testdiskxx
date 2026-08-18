/*

    File: btrfs.c

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

#include <cstdio>
#include <cstdlib>
#include <cstring>
// #include "types.h"
#include "btrfs.hpp"
#include "src/common.hpp"
#include "src/fnctdsk.hpp"
#include "src/guid_cpy.hpp"
#include "src/log.hpp"

static auto test_btrfs(const struct btrfs_super_block *sb) -> int;

static void set_btrfs_info(const struct btrfs_super_block *sb,
                           partition_t &partition)
{
  partition.upart_type = UP_BTRFS;
  partition.blocksize  = le32(sb->dev_item.sector_size);
  partition.set_name(sb->label, sizeof(sb->label));
  snprintf(partition.info, sizeof(partition.info), "btrfs blocksize=%u",
           partition.blocksize);
  if (le64(sb->bytenr) != partition.part_offset + BTRFS_SUPER_INFO_OFFSET)
  {
    strcat(partition.info, " Backup superblock");
  }
  /* last mounted => date */
}

auto check_btrfs(disk_t &disk_car, partition_t &partition) -> int
{
  auto *buffer = new unsigned char[BTRFS_SUPER_INFO_SIZE];
  if (disk_car.pread(disk_car, buffer, BTRFS_SUPER_INFO_SIZE,
                     partition.part_offset + BTRFS_SUPER_INFO_OFFSET) !=
      BTRFS_SUPER_INFO_SIZE)
  {
    delete[] (buffer);
    return 1;
  }
  if (test_btrfs(reinterpret_cast<struct btrfs_super_block *>(buffer)) != 0)
  {
    delete[] (buffer);
    return 1;
  }
  set_btrfs_info(reinterpret_cast<struct btrfs_super_block *>(buffer),
                 partition);
  delete[] (buffer);
  return 0;
}

/*
Primary superblock is at 1024 (SUPERBLOCK_OFFSET)
Group 0 begin at s_first_data_block
*/
auto recover_btrfs(const disk_t &disk, const struct btrfs_super_block *sb,
                   partition_t &partition, const int verbose,
                   const int dump_ind) -> int
{
  if (test_btrfs(sb) != 0)
    return 1;
  if (dump_ind != 0)
  {
    log_info("\nbtrfs magic value at {}/{}/{}\n",
             offset2cylinder(disk, partition.part_offset),
             offset2head(disk, partition.part_offset),
             offset2sector(disk, partition.part_offset));
    ; // dump_log(sb, BTRFS_SUPER_INFO_SIZE);
  }
  set_btrfs_info(sb, partition);
  partition.part_type_i386 = P_LINUX;
  partition.part_type_mac  = PMAC_LINUX;
  partition.part_type_sun  = PSUN_LINUX;
  partition.part_type_gpt  = GPT_ENT_TYPE_LINUX_DATA;
  partition.part_size      = le64(sb->dev_item.total_bytes);
  guid_cpy(&partition.part_uuid,
           reinterpret_cast<const efi_guid_t *>(&sb->fsid));
  if (verbose > 0)
  {
    log_info("\n");
  }
  partition.sborg_offset = BTRFS_SUPER_INFO_OFFSET;
  partition.sb_size      = BTRFS_SUPER_INFO_SIZE;
  if (verbose > 0)
  {
    // if (disk == NULL)
    //     log_info("recover_btrfs: part_size {}\n",
    //              (long unsigned)(partition.part_size /
    //              le32(sb->dev_item.sector_size)));
    // else
    log_info("recover_btrfs: part_size {}\n",
             (long unsigned)(partition.part_size / disk.sector_size));
  }
  return 0;
}

static auto test_btrfs(const struct btrfs_super_block *sb) -> int
{
  if (memcmp(&sb->magic, BTRFS_MAGIC, 8) != 0)
    return 1;
  if (le32(sb->dev_item.sector_size) == 0)
    return 1;
  return 0;
}
