/*

    File: md.c

    Copyright (C) 1998-2008 Christophe GRENIER <grenier@cgsecurity.org>

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
#include <format>
#include <string_view>
// #include "types.h"
#include "md.hpp"
#include "src/common.hpp"
#include "src/fnctdsk.hpp"
#include "src/log.hpp"

#ifndef DISABLED_FOR_FRAMAC
static auto test_MD(const disk_t &disk_car, const struct mdp_superblock_t *sb,
                    const partition_t &partition, const int dump_ind) -> int
{
  if (le32(sb->md_magic) != MD_SB_MAGIC)
    return 1;
  log_info("\nRaid magic value at {}/{}/{}\n",
           offset2cylinder(disk_car, partition.part_offset),
           offset2head(disk_car, partition.part_offset),
           offset2sector(disk_car, partition.part_offset));
  log_info("Raid apparent size: {} sectors\n",
           (long long unsigned)(sb->size << 1));
  if (le32(sb->major_version) == 0)
  {
    /* chunk_size may be 0 */
    log_info("Raid chunk size: {} bytes\n",
             (long long unsigned)le32(sb->chunk_size));
  }
  if (le32(sb->major_version) > 1)
    return 1;
  if (dump_ind != 0)
  {
    /* There is a little offset ... */
    ; // dump_log(sb,DEFAULT_SECTOR_SIZE);
  }
  return 0;
}

static auto test_MD_be(const disk_t &disk_car,
                       const struct mdp_superblock_t *sb,
                       const partition_t &partition, const int dump_ind) -> int
{
  if (be32(sb->md_magic) != MD_SB_MAGIC)
    return 1;
  log_info("\nRaid magic value at {}/{}/{}\n",
           offset2cylinder(disk_car, partition.part_offset),
           offset2head(disk_car, partition.part_offset),
           offset2sector(disk_car, partition.part_offset));
  log_info("Raid apparent size: {} sectors\n",
           (long long unsigned)(sb->size << 1));
  if (be32(sb->major_version) == 0)
  {
    /* chunk_size may be 0 */
    log_info("Raid chunk size: {} bytes\n",
             (long long unsigned)be32(sb->chunk_size));
  }
  if (be32(sb->major_version) > 1)
    return 1;
  if (dump_ind != 0)
  {
    /* There is a little offset ... */
    ; // dump_log(sb,DEFAULT_SECTOR_SIZE);
  }
  return 0;
}

static void set_MD_info(const struct mdp_superblock_t *sb,
                        partition_t &partition, const int verbose)
{
  if (le32(sb->major_version) == 0)
  {
    unsigned int i;
    partition.upart_type = UP_MD;
    partition.fsname     = std::format("md{}", le32(sb->md_minor));
    partition.info =
        std::format("md {}.{}.{} L.Endian Raid {}: devices",
                    le32(sb->major_version), le32(sb->minor_version),
                    le32(sb->patch_version), le32(sb->level));
    for (i = 0; i < MD_SB_DISKS; i++)
    {
      if (le32(sb->disks[i].major) != 0 && le32(sb->disks[i].minor) != 0)
      {
        partition.info +=
            std::format(" {}({},{})", le32(sb->disks[i].number),
                        le32(sb->disks[i].major), le32(sb->disks[i].minor));
        if (le32(sb->disks[i].major) == le32(sb->this_disk.major) &&
            le32(sb->disks[i].minor) == le32(sb->this_disk.minor))
          partition.info += "*";
      }
    }
  }
  else
  {
    const auto *sb1 = reinterpret_cast<const struct mdp_superblock_1 *>(sb);
    partition.upart_type = UP_MD1;
    partition.set_name(std::string_view(sb1->set_name, 32));
    partition.info = std::format("md {}.x L.Endian Raid {} - Array Slot : {}",
                                 le32(sb1->major_version), le32(sb1->level),
                                 le32(sb1->dev_number));
    if (le32(sb1->max_dev) <= 384)
    {
      unsigned int i, d;
      for (i = le32(sb1->max_dev); i > 0; i--)
        if (le16(sb1->dev_roles[i - 1]) != 0xffff)
          break;
      partition.info += " (";
      for (d = 0; d < i; d++)
      {
        const int role = le16(sb1->dev_roles[d]);
        if (d)
          partition.info += ", ";
        if (role == 0xffff)
          partition.info += "empty";
        else if (role == 0xfffe)
          partition.info += "failed";
        else
          partition.info += std::to_string(role);
      }
      partition.info += ")";
    }
  }
  if (verbose > 0)
    log_info("%s %s\n", partition.fsname, partition.info);
}

static void set_MD_info_be(const struct mdp_superblock_t *sb,
                           partition_t &partition, const int verbose)
{
  if (be32(sb->major_version) == 0)
  {
    unsigned int i;
    partition.upart_type = UP_MD;
    partition.fsname     = std::format("md{}", be32(sb->md_minor));
    partition.info =
        std::format("md {}.{}.{} B.Endian Raid {}: devices",
                    be32(sb->major_version), be32(sb->minor_version),
                    be32(sb->patch_version), be32(sb->level));
    for (i = 0; i < MD_SB_DISKS; i++)
    {
      if (be32(sb->disks[i].major) != 0 && be32(sb->disks[i].minor) != 0)
      {
        partition.info +=
            std::format(" {}({},{})", be32(sb->disks[i].number),
                        be32(sb->disks[i].major), be32(sb->disks[i].minor));
        if (be32(sb->disks[i].major) == be32(sb->this_disk.major) &&
            be32(sb->disks[i].minor) == be32(sb->this_disk.minor))
          partition.info += "*";
      }
    }
  }
  else
  {
    const auto *sb1 = reinterpret_cast<const struct mdp_superblock_1 *>(sb);
    partition.upart_type = UP_MD1;
    partition.set_name(std::string_view(sb1->set_name, 32));
    partition.info = std::format("md {}.x B.Endian Raid {} - Array Slot : {}",
                                 be32(sb1->major_version), be32(sb1->level),
                                 be32(sb1->dev_number));
    if (be32(sb1->max_dev) <= 384)
    {
      unsigned int i, d;
      for (i = be32(sb1->max_dev); i > 0; i--)
        if (be16(sb1->dev_roles[i - 1]) != 0xffff)
          break;
      partition.info += " (";
      for (d = 0; d < i; d++)
      {
        const int role = be16(sb1->dev_roles[d]);
        if (d)
          partition.info += ", ";
        if (role == 0xffff)
          partition.info += "empty";
        else if (role == 0xfffe)
          partition.info += "failed";
        else
          partition.info += std::to_string(role);
      }
      partition.info += ")";
    }
  }
  if (verbose > 0)
    log_info("%s %s\n", partition.fsname, partition.info);
}
#endif

auto check_MD(disk_t &disk_car, partition_t &partition, const int verbose)
    -> int
{
#ifndef DISABLED_FOR_FRAMAC
  auto *buffer = new unsigned char[MD_SB_BYTES];
  /* MD version 1.1 */
  if (disk_car.pread(disk_car, buffer, MD_SB_BYTES, partition.part_offset) ==
      MD_SB_BYTES)
  {
    const auto *sb1 = reinterpret_cast<const struct mdp_superblock_1 *>(buffer);
    if (le32(sb1->md_magic) == MD_SB_MAGIC && le32(sb1->major_version) == 1 &&
        le64(sb1->super_offset) == 0 &&
        test_MD(disk_car, reinterpret_cast<struct mdp_superblock_t *>(buffer),
                partition, 0) == 0)
    {
      log_info("check_MD 1.1\n");
      set_MD_info(reinterpret_cast<struct mdp_superblock_t *>(buffer),
                  partition, verbose);
      delete[] buffer;
      return 0;
    }
    if (be32(sb1->md_magic) == MD_SB_MAGIC && be32(sb1->major_version) == 1 &&
        be64(sb1->super_offset) == 0 &&
        test_MD_be(disk_car,
                   reinterpret_cast<struct mdp_superblock_t *>(buffer),
                   partition, 0) == 0)
    {
      log_info("check_MD 1.1 (BigEndian)\n");
      set_MD_info_be(reinterpret_cast<struct mdp_superblock_t *>(buffer),
                     partition, verbose);
      delete[] buffer;
      return 0;
    }
  }
  /* MD version 1.2 */
  if (disk_car.pread(disk_car, buffer, MD_SB_BYTES,
                     partition.part_offset + 4096) == MD_SB_BYTES)
  {
    const auto *sb1 = reinterpret_cast<const struct mdp_superblock_1 *>(buffer);
    if (le32(sb1->md_magic) == MD_SB_MAGIC && le32(sb1->major_version) == 1 &&
        le64(sb1->super_offset) == 8 &&
        test_MD(disk_car, reinterpret_cast<struct mdp_superblock_t *>(buffer),
                partition, 0) == 0)
    {
      log_info("check_MD 1.2\n");
      set_MD_info(reinterpret_cast<struct mdp_superblock_t *>(buffer),
                  partition, verbose);
      delete[] buffer;
      return 0;
    }
    if (be32(sb1->md_magic) == MD_SB_MAGIC && be32(sb1->major_version) == 1 &&
        be64(sb1->super_offset) == 8 &&
        test_MD_be(disk_car,
                   reinterpret_cast<struct mdp_superblock_t *>(buffer),
                   partition, 0) == 0)
    {
      log_info("check_MD 1.2 (BigEndian)\n");
      set_MD_info_be(reinterpret_cast<struct mdp_superblock_t *>(buffer),
                     partition, verbose);
      delete[] buffer;
      return 0;
    }
  }
  /* MD version 0.90 */
  {
    const auto *sb = reinterpret_cast<const struct mdp_superblock_t *>(buffer);
    const uint64_t offset =
        MD_NEW_SIZE_SECTORS(partition.part_size / 512) * 512;
    if (verbose > 1)
    {
      ; // log_verbose("Raid md 0.90 offset {}\n", (long long
        // unsigned)offset/512);
    }
    if (disk_car.pread(disk_car, buffer, MD_SB_BYTES,
                       partition.part_offset + offset) == MD_SB_BYTES)
    {
      if (le32(sb->md_magic) == MD_SB_MAGIC && le32(sb->major_version) == 0 &&
          test_MD(disk_car, reinterpret_cast<struct mdp_superblock_t *>(buffer),
                  partition, 0) == 0)
      {
        log_info("check_MD 0.90\n");
        set_MD_info(reinterpret_cast<struct mdp_superblock_t *>(buffer),
                    partition, verbose);
        delete[] buffer;
        return 0;
      }
      if (be32(sb->md_magic) == MD_SB_MAGIC && be32(sb->major_version) == 0 &&
          test_MD_be(disk_car,
                     reinterpret_cast<struct mdp_superblock_t *>(buffer),
                     partition, 0) == 0)
      {
        log_info("check_MD 0.90 (BigEndian)\n");
        set_MD_info_be(reinterpret_cast<struct mdp_superblock_t *>(buffer),
                       partition, verbose);
        delete[] buffer;
        return 0;
      }
    }
  }
  /* MD version 1.0 */
  if (partition.part_size > 8 * 2 * 512)
  {
    const uint64_t offset =
        (((partition.part_size / 512) - 8 * 2) & ~(4 * 2 - 1)) * 512;
    if (verbose > 1)
    {
      ; // log_verbose("Raid md 1.0 offset {}\n", (long long
        // unsigned)offset/512);
    }
    if (disk_car.pread(disk_car, buffer, MD_SB_BYTES,
                       partition.part_offset + offset) == MD_SB_BYTES)
    {
      const auto *sb1 =
          reinterpret_cast<const struct mdp_superblock_1 *>(buffer);
      if (le32(sb1->md_magic) == MD_SB_MAGIC && le32(sb1->major_version) == 1 &&
          le64(sb1->super_offset) == (offset / 512) &&
          test_MD(disk_car, reinterpret_cast<struct mdp_superblock_t *>(buffer),
                  partition, 0) == 0)
      {
        log_info("check_MD 1.0\n");
        set_MD_info(reinterpret_cast<struct mdp_superblock_t *>(buffer),
                    partition, verbose);
        delete[] buffer;
        return 0;
      }
      if (be32(sb1->md_magic) == MD_SB_MAGIC && be32(sb1->major_version) == 1 &&
          be64(sb1->super_offset) == (offset / 512) &&
          test_MD_be(disk_car,
                     reinterpret_cast<struct mdp_superblock_t *>(buffer),
                     partition, 0) == 0)
      {
        log_info("check_MD 1.0 (BigEndian)\n");
        set_MD_info_be(reinterpret_cast<struct mdp_superblock_t *>(buffer),
                       partition, verbose);
        delete[] buffer;
        return 0;
      }
    }
  }
  delete[] buffer;
#endif
  return 1;
}

auto recover_MD_from_partition(disk_t &disk_car, partition_t &partition,
                               const int verbose) -> int
{
#ifndef DISABLED_FOR_FRAMAC
  auto *buffer = new unsigned char[MD_SB_BYTES];
  /* MD version 0.90 */
  {
    const uint64_t offset =
        MD_NEW_SIZE_SECTORS(partition.part_size / 512) * 512;
    if (disk_car.pread(disk_car, buffer, MD_SB_BYTES,
                       partition.part_offset + offset) == MD_SB_BYTES)
    {
      if (recover_MD(disk_car,
                     reinterpret_cast<struct mdp_superblock_t *>(buffer),
                     partition, verbose, 0) == 0)
      {
        delete[] buffer;
        return 0;
      }
    }
  }
  /* MD version 1.0 */
  if (partition.part_size > 8 * 2 * 512)
  {
    const uint64_t offset =
        (((partition.part_size / 512) - 8 * 2) & ~(4 * 2 - 1)) * 512;
    if (disk_car.pread(disk_car, buffer, MD_SB_BYTES,
                       partition.part_offset + offset) == MD_SB_BYTES)
    {
      const auto *sb1 =
          reinterpret_cast<const struct mdp_superblock_1 *>(buffer);
      if (le32(sb1->major_version) == 1 &&
          recover_MD(disk_car,
                     reinterpret_cast<struct mdp_superblock_t *>(buffer),
                     partition, verbose, 0) == 0)
      {
        partition.part_offset -= le64(sb1->super_offset) * 512 - offset;
        delete[] buffer;
        return 0;
      }
    }
  }
  /* md 1.1 & 1.2 don't need special operation to be recovered */
  delete[] buffer;
#endif
  return 1;
}

auto recover_MD(const disk_t &disk_car, const struct mdp_superblock_t *sb,
                partition_t &partition, const int verbose, const int dump_ind)
    -> int
{
#ifndef DISABLED_FOR_FRAMAC
  if (test_MD(disk_car, sb, partition, dump_ind) == 0)
  {
    set_MD_info(sb, partition, verbose);
    partition.part_type_i386 = P_RAID;
    partition.part_type_sun  = PSUN_RAID;
    partition.part_type_gpt  = GPT_ENT_TYPE_LINUX_RAID;
    if (le32(sb->major_version) == 0)
    {
      partition.part_size = static_cast<uint64_t>(le32(sb->size) << 1) * 512 +
                            MD_RESERVED_BYTES; /* 512-byte sectors */
      memcpy(&partition.part_uuid, &sb->set_uuid0, 4);
      memcpy(reinterpret_cast<char *>(&partition.part_uuid) + 4, &sb->set_uuid1,
             3 * 4);
    }
    else
    {
      const auto *sb1 = reinterpret_cast<const struct mdp_superblock_1 *>(sb);
      partition.part_size = le64(sb1->size) * 512 + 4096; /* 512-byte sectors */
      memcpy(&partition.part_uuid, &sb1->set_uuid, 16);
    }
    return 0;
  }
  if (test_MD_be(disk_car, sb, partition, dump_ind) == 0)
  {
    set_MD_info_be(sb, partition, verbose);
    partition.part_type_i386 = P_RAID;
    partition.part_type_sun  = PSUN_RAID;
    partition.part_type_gpt  = GPT_ENT_TYPE_LINUX_RAID;
    if (be32(sb->major_version) == 0)
    {
      partition.part_size = static_cast<uint64_t>(be32(sb->size) << 1) * 512 +
                            MD_RESERVED_BYTES; /* 512-byte sectors */
      memcpy(&partition.part_uuid, &sb->set_uuid0, 4);
      memcpy(reinterpret_cast<char *>(&partition.part_uuid) + 4, &sb->set_uuid1,
             3 * 4);
    }
    else
    {
      const auto *sb1 = reinterpret_cast<const struct mdp_superblock_1 *>(sb);
      partition.part_size = static_cast<uint64_t>(be64(sb1->size)) * 512 +
                            4096; /* 512-byte sectors */
      memcpy(&partition.part_uuid, &sb1->set_uuid, 16);
    }
    return 0;
  }
#endif
  return 1;
}
