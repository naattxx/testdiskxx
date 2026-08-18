/*

    File: wbfs.c

    Copyright (C) 2012 Christophe GRENIER <grenier@cgsecurity.org>

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
#include "src/common.hpp"
#include "src/fnctdsk.hpp"
#include "src/log.hpp"
#include "wbfs.hpp"

static auto test_WBFS(const disk_t &disk, const struct wbfs_head *sb,
                      const partition_t &partition, const int dump_ind) -> int
{
  if (be32(sb->magic) != WBFS_MAGIC)
    return 1;
  if (dump_ind != 0)
  {
    log_info("\nWBFS magic value at {}/{}/{}\n",
             offset2cylinder(disk, partition.part_offset),
             offset2head(disk, partition.part_offset),
             offset2sector(disk, partition.part_offset));
    ; // dump_log(sb,DEFAULT_SECTOR_SIZE);
  }
  return 0;
}

static void set_WBFS_info(partition_t &partition)
{
  partition.upart_type = UP_WBFS;
  partition.info       = "WBFS";
}

auto check_WBFS(disk_t &disk, partition_t &partition) -> int
{
  auto *buffer = new unsigned char[2 * DEFAULT_SECTOR_SIZE];
  if (disk.pread(disk, buffer, 2 * DEFAULT_SECTOR_SIZE,
                 partition.part_offset + 0x100000) != DEFAULT_SECTOR_SIZE)
  {
    delete[] (buffer);
    return 1;
  }
  if (test_WBFS(disk, reinterpret_cast<struct wbfs_head *>(buffer), partition,
                0) != 0)
  {
    delete[] (buffer);
    return 1;
  }
  set_WBFS_info(partition);
  delete[] (buffer);
  return 0;
}

auto recover_WBFS(const disk_t &disk, const struct wbfs_head *sb,
                  partition_t &partition, const int verbose, const int dump_ind)
    -> int
{
  if (test_WBFS(disk, sb, partition, dump_ind) != 0)
    return 1;
  set_WBFS_info(partition);
  partition.part_type_i386 = P_NTFS;
  partition.part_size      = static_cast<uint64_t>(be32(sb->n_hd_sec))
                          << (sb->hd_sec_sz_s);
  partition.blocksize      = 0;
  partition.sborg_offset   = 0;
  partition.sb_offset      = 0;
  if (verbose > 0)
  {
    log_info("\n");
  }
  return 0;
}
