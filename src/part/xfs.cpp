/*

    File: xfs.c

    Copyright (C) 2004-2007 Christophe GRENIER <grenier@cgsecurity.org>

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
#include "src/guid_cpy.hpp"
#include "src/log.hpp"
#include "xfs.hpp"

static void set_xfs_info(const struct xfs_sb *sb, partition_t &partition)
{
  const unsigned int version = be16(sb->sb_versionnum) & XFS_SB_VERSION_NUMBITS;
  partition.blocksize        = be32(sb->sb_blocksize);
  partition.fsname.clear();
  partition.info.clear();
  switch (version)
  {
  case XFS_SB_VERSION_1:
    partition.upart_type = UP_XFS;
    partition.info =
        std::format("XFS <=6.1, blocksize={}", partition.blocksize);
    break;
  case XFS_SB_VERSION_2:
    partition.upart_type = UP_XFS2;
    partition.info =
        std::format("XFS 6.2 - attributes, blocksize={}", partition.blocksize);
    break;
  case XFS_SB_VERSION_3:
    partition.upart_type = UP_XFS3;
    partition.info = std::format("XFS 6.2 - new inode version, blocksize={}",
                                 partition.blocksize);
    break;
  case XFS_SB_VERSION_4:
    partition.upart_type = UP_XFS4;
    partition.info = std::format("XFS 6.2+ - bitmap version, blocksize={}",
                                 partition.blocksize);
    break;
  case XFS_SB_VERSION_5:
    partition.upart_type = UP_XFS5;
    partition.info =
        std::format("XFS CRC enabled, blocksize={}", partition.blocksize);
    break;
  default:
    partition.info = std::format("XFS unknown version {}", version);
    break;
  }
  partition.set_name(sb->sb_fname, 12);
}

static auto test_xfs(const disk_t &disk_car, const struct xfs_sb *sb,
                     const partition_t &partition, const int verbose) -> int
{
  if (sb->sb_magicnum != be32(XFS_SB_MAGIC) ||
      static_cast<uint16_t>(be16(sb->sb_sectsize)) != (1U << sb->sb_sectlog) ||
      static_cast<uint32_t>(be32(sb->sb_blocksize)) !=
          (1U << sb->sb_blocklog) ||
      static_cast<uint16_t>(be16(sb->sb_inodesize)) != (1U << sb->sb_inodelog))
    return 1;
  switch (be16(sb->sb_versionnum) & XFS_SB_VERSION_NUMBITS)
  {
  case XFS_SB_VERSION_1:
  case XFS_SB_VERSION_2:
  case XFS_SB_VERSION_3:
  case XFS_SB_VERSION_4:
  case XFS_SB_VERSION_5:
    break;
  default:
    log_error("Unknown XFS version 0x%x\n",
              be16(sb->sb_versionnum) & XFS_SB_VERSION_NUMBITS);
    break;
  }
  if (verbose > 0)
    log_info("\nXFS Marker at {}/{}/{}\n",
             offset2cylinder(disk_car, partition.part_offset),
             offset2head(disk_car, partition.part_offset),
             offset2sector(disk_car, partition.part_offset));
  return 0;
}

auto check_xfs(disk_t &disk_car, partition_t &partition, const int verbose)
    -> int
{
  auto *buffer = new unsigned char[XFS_SUPERBLOCK_SIZE];
  if (disk_car.pread(disk_car, buffer, XFS_SUPERBLOCK_SIZE,
                     partition.part_offset) != XFS_SUPERBLOCK_SIZE)
  {
    delete[] (buffer);
    return 1;
  }
  if (test_xfs(disk_car, reinterpret_cast<struct xfs_sb *>(buffer), partition,
               verbose) != 0)
  {
    delete[] (buffer);
    return 1;
  }
  set_xfs_info(reinterpret_cast<struct xfs_sb *>(buffer), partition);
  delete[] (buffer);
  return 0;
}

auto recover_xfs(const disk_t &disk_car, const struct xfs_sb *sb,
                 partition_t &partition, const int verbose, const int dump_ind)
    -> int
{
  if (test_xfs(disk_car, sb, partition, verbose) != 0)
    return 1;
  if (verbose > 0 || dump_ind != 0)
  {
    log_info("\nrecover_xfs\n");
    if (dump_ind != 0)
    {
      ; // dump_log(sb,DEFAULT_SECTOR_SIZE);
    }
  }
  set_xfs_info(sb, partition);
  partition.part_size =
      static_cast<uint64_t>(be64(sb->sb_dblocks)) * be32(sb->sb_blocksize);
  partition.part_type_i386 = P_LINUX;
  partition.part_type_mac  = PMAC_LINUX;
  partition.part_type_sun  = PSUN_LINUX;
  partition.part_type_gpt  = GPT_ENT_TYPE_LINUX_DATA;
  guid_cpy(&partition.part_uuid,
           reinterpret_cast<const efi_guid_t *>(&sb->sb_uuid));
  return 0;
}
