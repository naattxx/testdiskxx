/*
    File: ntfs_fix.c - Part of the TestDisk project.

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
#include "ntfs_fix.hpp"
#include "ntfs.hpp"
#include "ntfs_dir.hpp"
#include "src/common.hpp"
#include "src/dir.hpp"
#include "src/dir_common.hpp"
#include "src/io_redir.hpp"
#include "src/log.hpp"
#include "src/part/ntfs_struct.hpp"
#include <algorithm>
#include <config.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <expected>
#include <new>
#include <span>
#include <string_view>
#include <utility>

// #define DEBUG_REPAIR_MFT 1

auto mft_state::fix_mft() -> std::string_view
{
  if (std::cmp_equal(disk.pwrite(disk, buffer_mftmirr.data(),
                                 buffer_mftmirr.size(), mft_pos),
                     buffer_mftmirr.size()))
  {
    disk.sync(disk);
    return "MFT fixed.";
  }
  return "Failed to fix MFT: write error.";
}
auto mft_state::fix_mft_mirror() -> std::string_view
{
  if (std::cmp_equal(disk.pwrite(disk, buffer_mft.data(), buffer_mft.size(),
                                 mftmirr_pos),
                     buffer_mft.size()))
  {
    disk.sync(disk);
    return "MFT mirror fixed.";
  }
  return "Failed to fix MFT mirror: write error.";
}

auto mft_state::get(disk_t &disk, partition_t &partition, const int verbose,
                      const bool expert)
    -> std::expected<mft_state, std::string_view>
{
  std::byte *buffer_mft;
  std::byte *buffer_mftmirr;
  unsigned int cluster_size;
  unsigned int mft_record_size;
  unsigned int mftmirr_size_bytes;
  uint64_t mft_pos;
  uint64_t mftmirr_pos;
  log_trace("repair_MFT");
  if (check_NTFS(disk, partition, verbose, false) != 0)
  {
    return std::unexpected("Boot sector not valid, can't repair MFT.");
  }

  auto *ntfs_header = new (std::nothrow) struct ntfs_boot_sector;
  if (ntfs_header == nullptr)
    return std::unexpected("Run out of memory.");

  if (disk.pread(disk, ntfs_header, DEFAULT_SECTOR_SIZE,
                 partition.part_offset) != DEFAULT_SECTOR_SIZE)
  {
    delete ntfs_header;
    return std::unexpected("Can't read NTFS boot sector.");
  }
  mft_pos = partition.part_offset + (to_little_endian(ntfs_header->reserved) +
                                     to_little_endian(ntfs_header->mft_lcn) *
                                         ntfs_header->sectors_per_cluster) *
                                        ntfs_sector_size(ntfs_header);
  mftmirr_pos =
      partition.part_offset + (to_little_endian(ntfs_header->reserved) +
                               to_little_endian(ntfs_header->mftmirr_lcn) *
                                   ntfs_header->sectors_per_cluster) *
                                  ntfs_sector_size(ntfs_header);
  if (ntfs_header->clusters_per_mft_record > 0)
    mft_record_size = ntfs_header->clusters_per_mft_record *
                      ntfs_header->sectors_per_cluster *
                      ntfs_sector_size(ntfs_header);
  else
    mft_record_size = 1u << static_cast<uint8_t>(-ntfs_header->clusters_per_mft_record);

  if (mft_record_size < 42)
  {
    log_error("Invalid NTFS MFT record size.");
    delete ntfs_header;
    return std::unexpected("Invalid NTFS MFT record size.");
  }

  cluster_size =
      ntfs_header->sectors_per_cluster * ntfs_sector_size(ntfs_header);

  mftmirr_size_bytes = std::max(cluster_size, 4 * mft_record_size);
#ifdef DEBUG_REPAIR_MFT
  log_info("mft_pos          {}", mft_pos / disk.sector_size);
  log_info("mftmirr_pos      {}", mftmirr_pos / disk.sector_size);
  log_info("cluster_size     {:5} bytes", cluster_size);
  log_info("mft_record_size  {:5} bytes", mft_record_size);
  log_info("ntfs_sector_size {:5} bytes", ntfs_sector_size(ntfs_header));
  log_info("mftmirr_size     {:5} bytes", mftmirr_size_bytes);
#endif

  delete ntfs_header;

  if (mftmirr_size_bytes == 0)
  {
    log_error("Invalid NTFS MFT size.");
    return std::unexpected("Invalid NTFS MFT size.");
  }
  /* Check if MFT mirror is identical to the beginning of MFT */
  buffer_mft = new (std::nothrow) std::byte[mftmirr_size_bytes];
  if (buffer_mft == nullptr)
    return std::unexpected("Run out of memory.");

  if (std::cmp_not_equal(disk.pread(disk, buffer_mft, mftmirr_size_bytes,
                                    mft_pos),
                         mftmirr_size_bytes))
  {
    log_error("Can't read NTFS MFT.");
    delete[] buffer_mft;
    return std::unexpected("Can't read NTFS MFT.");
  }
  buffer_mftmirr = new(std::nothrow) std::byte[mftmirr_size_bytes];
  if (buffer_mftmirr == nullptr)
  {
    delete[] buffer_mft;
    return std::unexpected("Run out of memory.");
  }
  if (std::cmp_not_equal(disk.pread(disk, buffer_mftmirr, mftmirr_size_bytes,
                                    mftmirr_pos),
                         mftmirr_size_bytes))
  {
    log_error("Can't read NTFS MFT mirror.");
    delete[] buffer_mftmirr;
    delete[] buffer_mft;
    return std::unexpected("Can't read NTFS MFT mirror.");
  }
  if (memcmp(buffer_mft, buffer_mftmirr, mftmirr_size_bytes) == 0)
  {
    log_info("MFT and MFT mirror match perfectly.");
    delete[] buffer_mftmirr;
    delete[] buffer_mft;
    return std::unexpected("MFT and MFT mirror match perfectly.");
  }
  if (partition.sb_offset != 0)
  {
    log_info(
        "Please quit TestDisk and reboot your computer before trying to fix "
        "the MFT."
    );
    delete[] buffer_mftmirr;
    delete[] buffer_mft;
    return std::unexpected(
        "Please quit TestDisk and reboot your computer before trying to fix "
        "the MFT."
    );
  }
#ifdef DEBUG_REPAIR_MFT
  // log_info("MFT");
  // dump_log(buffer_mft, mftmirr_size_bytes);
  // log_info("MFT mirror");
  // dump_log(buffer_mftmirr, mftmirr_size_bytes);
  // log_flush();
#endif
  /*
  The idea is to use the internal IO redirector built-in TestDisk
  to redirect read access to the MFT to the MFT backup instead (or
  vice-versa) when listing the NTFS files. If TestDisk can get
  a file listing, it also knows which MFT to use.
  */
  auto expected_dir_data = dir_partition_ntfs_init(disk, partition, verbose, false);
  bool mft_is_good = false, mft_mirror_is_good = false;
  dir_data_t *dir_data;
  /* Use MFT */
  io_redir_add_redir(disk, mftmirr_pos, mftmirr_size_bytes, 0, buffer_mft);
  if (expected_dir_data.error_or(DIR_PART_OK) == DIR_PART_ENOSYS)
  {
    log_error("Can't determine which MFT is correct, ntfslib is missing.");
    delete[] buffer_mftmirr;
    delete[] buffer_mft;
    io_redir_del_redir(disk, mftmirr_pos);
    return std::unexpected(
        "Can't determine which MFT is correct, ntfslib is missing."
    );
  }
  if (expected_dir_data)
  {
    dir_list_t dir_list;
    dir_data = *expected_dir_data;
    dir_data->get_dir(disk, partition, dir_data->current_inode, dir_list);
    if (!dir_list.empty())
    {
      log_info("NTFS listing using MFT:");
      dir_aff_log(*dir_data, dir_list);
      if (dir_list.size() > 2)
        mft_is_good = true;
    }
    dir_data->close();
    delete dir_data;
  }
  io_redir_del_redir(disk, mftmirr_pos);
  /* Use MFT mirror */
  io_redir_add_redir(disk, mft_pos, mftmirr_size_bytes, 0, buffer_mftmirr);
  expected_dir_data = dir_partition_ntfs_init(disk, partition, verbose, false);
  if (expected_dir_data)
  {
    dir_list_t dir_list;
    dir_data = *expected_dir_data;
    dir_data->get_dir(disk, partition, dir_data->current_inode, dir_list);
    if (!dir_list.empty())
    {
      log_info("NTFS listing using MFT mirror:");
      dir_aff_log(*dir_data, dir_list);
      if (dir_list.size() > 2)
        mft_mirror_is_good = true;
    }
    dir_data->close();
    delete dir_data;
  }
  io_redir_del_redir(disk, mft_pos);

  if (!expert && !mft_is_good && !mft_mirror_is_good)
    return std::unexpected(
        "MFT and MFT mirror are bad. Failed to repair them."
    );

  if (!expert && mft_is_good && mft_mirror_is_good)
    return std::unexpected(
        "Both MFT seems ok but they don't match, use chkdsk."
    );

  return {mft_state(disk, std::span(buffer_mft, mftmirr_size_bytes),
                    std::span(buffer_mftmirr, mftmirr_size_bytes), mft_pos,
                    mftmirr_pos, mft_is_good, mft_mirror_is_good)};
}
