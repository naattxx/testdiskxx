/*

    File: ufs.c

    Copyright (C) 2005-2007 Christophe GRENIER <grenier@cgsecurity.org>

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
#include <string_view>
#include "src/common.hpp"
#include "src/fnctdsk.hpp"
#include "src/log.hpp"
#include "ufs.hpp"

static void set_ufs_info(const struct ufs_super_block *sb,
                         partition_t &partition);
static auto test_ufs(const disk_t &disk_car, const struct ufs_super_block *sb,
                     const partition_t &partition, const int verbose) -> int;

auto check_ufs(disk_t &disk_car, partition_t &partition, const int verbose)
    -> int
{
  const struct ufs_super_block *sb;
  unsigned char *buffer;
  buffer = new unsigned char[UFS_SUPERBLOCK_SIZE];
  sb     = reinterpret_cast<const struct ufs_super_block *>(buffer);
  if (disk_car.pread(disk_car, buffer, UFS_SUPERBLOCK_SIZE,
                     partition.part_offset + UFS_SBLOCK) != UFS_SUPERBLOCK_SIZE)
  {
    delete[] buffer;
    return 1;
  }
  if (test_ufs(disk_car, sb, partition, verbose) != 0)
  {
    delete[] buffer;
    return 1;
  }
  set_ufs_info(sb, partition);
  delete[] buffer;
  return 0;
}

static auto test_ufs(const disk_t &disk_car, const struct ufs_super_block *sb,
                     const partition_t &partition, const int verbose) -> int
{
  if (to_little_endian(sb->fs_magic) == UFS_MAGIC && to_little_endian(sb->fs_size) > 0 &&
      (to_little_endian(sb->fs_fsize) == 512 || to_little_endian(sb->fs_fsize) == 1024 ||
       to_little_endian(sb->fs_fsize) == 2048 || to_little_endian(sb->fs_fsize) == 4096))
  {
    if (verbose > 1)
      log_info("\nUFS Marker at {}/{}/{}\n",
               offset2cylinder(disk_car, partition.part_offset),
               offset2head(disk_car, partition.part_offset),
               offset2sector(disk_car, partition.part_offset));
    return 0;
  }
  if (to_big_endian(sb->fs_magic) == UFS_MAGIC && to_big_endian(sb->fs_size) > 0 &&
      (to_big_endian(sb->fs_fsize) == 512 || to_big_endian(sb->fs_fsize) == 1024 ||
       to_big_endian(sb->fs_fsize) == 2048 || to_big_endian(sb->fs_fsize) == 4096))
  {
    if (verbose > 1)
      log_info("\nUFS Marker at {}/{}/{}\n",
               offset2cylinder(disk_car, partition.part_offset),
               offset2head(disk_car, partition.part_offset),
               offset2sector(disk_car, partition.part_offset));
    return 0;
  }
  if (to_little_endian(sb->fs_magic) == UFS2_MAGIC && to_little_endian(sb->fs_u11.fs_u2.fs_size) > 0 &&
      (to_little_endian(sb->fs_fsize) == 512 || to_little_endian(sb->fs_fsize) == 1024 ||
       to_little_endian(sb->fs_fsize) == 2048 || to_little_endian(sb->fs_fsize) == 4096))
  {
    if (verbose > 1)
      log_info("\nUFS2 Marker at {}/{}/{}\n",
               offset2cylinder(disk_car, partition.part_offset),
               offset2head(disk_car, partition.part_offset),
               offset2sector(disk_car, partition.part_offset));
    return 0;
  }
  if (to_big_endian(sb->fs_magic) == UFS2_MAGIC && to_big_endian(sb->fs_u11.fs_u2.fs_size) > 0 &&
      (to_big_endian(sb->fs_fsize) == 512 || to_big_endian(sb->fs_fsize) == 1024 ||
       to_big_endian(sb->fs_fsize) == 2048 || to_big_endian(sb->fs_fsize) == 4096))
  {
    if (verbose > 1)
      log_info("\nUFS2 Marker at {}/{}/{}\n",
               offset2cylinder(disk_car, partition.part_offset),
               offset2head(disk_car, partition.part_offset),
               offset2sector(disk_car, partition.part_offset));
    return 0;
  }
  return 1;
}

auto recover_ufs(const disk_t &disk_car, const struct ufs_super_block *sb,
                 partition_t &partition, const int verbose, const int dump_ind)
    -> int
{
  if (test_ufs(disk_car, sb, partition, verbose) != 0)
    return 1;
  if (dump_ind != 0)
  {
    log_info("recover_ufs\n");
    ; // dump_log(sb,sizeof(*sb));
  }
  set_ufs_info(sb, partition);
  switch (partition.upart_type)
  {
  case UP_UFS_LE:
    partition.part_size =
        static_cast<uint64_t>(to_little_endian(sb->fs_size)) * to_little_endian(sb->fs_fsize);
    if (verbose > 1)
    {
      log_info("fs_size {}, fs_fsize {}\n", (long unsigned)to_little_endian(sb->fs_size),
               (long unsigned)to_little_endian(sb->fs_fsize));
      log_info("fs_sblkno {}\n", (long unsigned)to_little_endian(sb->fs_sblkno));
    }
    break;
  case UP_UFS2_LE:
    partition.part_size = to_little_endian(sb->fs_u11.fs_u2.fs_size) * to_little_endian(sb->fs_fsize);
    if (verbose > 1)
    {
      log_info("fs_size {}, fs_fsize {}\n",
               (long unsigned)to_little_endian(sb->fs_u11.fs_u2.fs_size),
               (long unsigned)to_little_endian(sb->fs_fsize));
      log_info("fs_sblkno {}\n", (long unsigned)to_little_endian(sb->fs_sblkno));
      log_info("fs_sblockloc {}\n",
               (long long unsigned)to_little_endian(sb->fs_u11.fs_u2.fs_sblockloc));
    }
    break;
  case UP_UFS:
    partition.part_size =
        static_cast<uint64_t>(to_big_endian(sb->fs_size)) * to_big_endian(sb->fs_fsize);
    if (verbose > 1)
    {
      log_info("fs_size {}, fs_fsize {}\n", (long unsigned)to_big_endian(sb->fs_size),
               (long unsigned)to_big_endian(sb->fs_fsize));
      log_info("fs_sblkno {}\n", (long unsigned)to_big_endian(sb->fs_sblkno));
    }
    break;
  case UP_UFS2:
    partition.part_size =
        static_cast<uint64_t>(to_big_endian(sb->fs_u11.fs_u2.fs_size)) *
        to_big_endian(sb->fs_fsize);
    if (verbose > 1)
    {
      log_info("fs_size {}, fs_fsize {}\n",
               (long unsigned)to_big_endian(sb->fs_u11.fs_u2.fs_size),
               (long unsigned)to_big_endian(sb->fs_fsize));
      log_info("fs_sblkno {}\n", (long unsigned)to_big_endian(sb->fs_sblkno));
      log_info("fs_sblockloc {}\n",
               (long long unsigned)to_big_endian(sb->fs_u11.fs_u2.fs_sblockloc));
    }
    break;
  default: /* BUG if hit*/
    break;
  }
  if (partition.fsname == "/")
  {
    partition.part_type_sun = static_cast<unsigned char>(PSUN_ROOT);
    partition.part_type_gpt = GPT_ENT_TYPE_SOLARIS_ROOT;
  }
  else if (partition.fsname == "/var")
  {
    partition.part_type_sun = static_cast<unsigned char>(PSUN_VAR);
    partition.part_type_gpt = GPT_ENT_TYPE_SOLARIS_VAR;
  }
  else if (partition.fsname == "/usr")
  {
    partition.part_type_sun = static_cast<unsigned char>(PSUN_USR);
    partition.part_type_gpt = GPT_ENT_TYPE_SOLARIS_USR;
  }
  else if (partition.fsname == "/export/home")
  {
    partition.part_type_sun = static_cast<unsigned char>(PSUN_HOME);
    partition.part_type_gpt = GPT_ENT_TYPE_SOLARIS_HOME;
  }
  else
  {
    partition.part_type_sun = static_cast<unsigned char>(PSUN_ROOT);
    partition.part_type_gpt = GPT_ENT_TYPE_SOLARIS_HOME;
  }
  return 0;
}

static void set_ufs_info(const struct ufs_super_block *sb,
                         partition_t &partition)
{
  partition.fsname[0] = '\0';
  partition.info[0]   = '\0';
  if (to_little_endian(sb->fs_magic) == UFS_MAGIC)
  {
    partition.upart_type = UP_UFS_LE;
    partition.blocksize  = to_little_endian(sb->fs_fsize);
    partition.set_name(
        std::string_view(reinterpret_cast<const char *>(sb->fs_u11.fs_u1.fs_fsmnt),
        sizeof(sb->fs_u11.fs_u1.fs_fsmnt))
    );
    partition.info = std::format("UFS1 blocksize={}", partition.blocksize);
  }
  if (to_big_endian(sb->fs_magic) == UFS_MAGIC)
  {
    partition.upart_type = UP_UFS;
    partition.blocksize  = to_big_endian(sb->fs_fsize);
    partition.set_name(
        std::string_view(reinterpret_cast<const char *>(sb->fs_u11.fs_u1.fs_fsmnt),
        sizeof(sb->fs_u11.fs_u1.fs_fsmnt))
    );
    partition.info = std::format("UFS1 blocksize={}", partition.blocksize);
  }
  if (to_little_endian(sb->fs_magic) == UFS2_MAGIC)
  {
    partition.blocksize  = to_little_endian(sb->fs_fsize);
    partition.upart_type = UP_UFS2_LE;
    partition.set_name(
        std::string_view(reinterpret_cast<const char *>(sb->fs_u11.fs_u2.fs_fsmnt),
        sizeof(sb->fs_u11.fs_u2.fs_fsmnt))
    );
    partition.info = std::format("UFS2 blocksize={}", partition.blocksize);
  }
  if (to_big_endian(sb->fs_magic) == UFS2_MAGIC)
  {
    partition.upart_type = UP_UFS2;
    partition.blocksize  = to_big_endian(sb->fs_fsize);
    partition.set_name(
        std::string_view(reinterpret_cast<const char *>(sb->fs_u11.fs_u2.fs_fsmnt),
        sizeof(sb->fs_u11.fs_u2.fs_fsmnt))
    );
    partition.info = std::format("UFS2 blocksize={}", partition.blocksize);
  }
}
