/*

    File: cramfs.c

    Copyright (C) 1998-2007 Christophe GRENIER <grenier@cgsecurity.org>

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

#include <array>
#include <config.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string_view>
#include "cramfs.hpp"
#include "src/common.hpp"
#include "src/fnctdsk.hpp"
#include "src/log.hpp"

static void set_cramfs_info(const struct cramfs_super *sb,
                            partition_t &partition);
static auto test_cramfs(const disk_t &disk_car, const struct cramfs_super *sb,
                        const partition_t &partition, const int verbose) -> int;

auto check_cramfs(disk_t &disk_car, partition_t &partition, const int verbose)
    -> int
{
  std::array<unsigned char, CRAMFS_SUPERBLOCK_SIZE> buffer;
  if (disk_car.pread(disk_car, buffer.data(), CRAMFS_SUPERBLOCK_SIZE,
                     partition.part_offset + 0x200) == CRAMFS_SUPERBLOCK_SIZE)
  {
    if (test_cramfs(disk_car, std::start_lifetime_as<struct cramfs_super>(buffer.data()),
                    partition, verbose) == 0)
    {
      set_cramfs_info(std::start_lifetime_as<struct cramfs_super>(buffer.data()),
                      partition);
      return 0;
    }
  }
  if (disk_car.pread(disk_car, buffer.data(), CRAMFS_SUPERBLOCK_SIZE,
                     partition.part_offset) == CRAMFS_SUPERBLOCK_SIZE)
  {
    if (test_cramfs(disk_car, std::start_lifetime_as<struct cramfs_super>(buffer.data()),
                    partition, verbose) == 0)
    {
      set_cramfs_info(std::start_lifetime_as<struct cramfs_super>(buffer.data()),
                      partition);
      return 0;
    }
  }
  return 1;
}

static auto test_cramfs(const disk_t &disk_car, const struct cramfs_super *sb,
                        const partition_t &partition, const int verbose) -> int
{
  if (sb->magic != to_little_endian(CRAMFS_MAGIC))
    return 1;
  if (verbose > 0)
    log_info("\ncramfs Marker at {}/{}/{}\n",
             offset2cylinder(disk_car, partition.part_offset),
             offset2head(disk_car, partition.part_offset),
             offset2sector(disk_car, partition.part_offset));
  return 0;
}

auto recover_cramfs(const disk_t &disk_car, const struct cramfs_super *sb,
                    partition_t &partition, const int verbose,
                    const bool dump_ind) -> int
{
  if (test_cramfs(disk_car, sb, partition, verbose) != 0)
    return 1;
  if (verbose > 0 || dump_ind)
  {
    log_trace("recover_cramfs");
    if (dump_ind)
    {
      ; // dump_log(sb,DEFAULT_SECTOR_SIZE);
    }
  }
  partition.part_size      = sb->size;
  partition.part_type_i386 = P_LINUX;
  partition.part_type_mac  = PMAC_LINUX;
  partition.part_type_sun  = PSUN_LINUX;
  partition.part_type_gpt  = GPT_ENT_TYPE_LINUX_DATA;
  set_cramfs_info(sb, partition);
  return 0;
}

static void set_cramfs_info(const struct cramfs_super *sb,
                            partition_t &partition)
{
  partition.upart_type = UP_CRAMFS;
  partition.set_name(std::string_view(reinterpret_cast<const char *>(sb->name), 16));
  partition.info = "cramfs";
}
