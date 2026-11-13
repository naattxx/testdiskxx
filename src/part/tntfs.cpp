/*

    File: tntfs.c

    Copyright (C) 2008 Christophe GRENIER <grenier@cgsecurity.org>

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
#include "tntfs.hpp"
#include "ntfs.hpp"
#include "ntfs_adv.hpp"
#include "src/common.hpp"
#include "src/log.hpp"
#include "src/log_part.hpp"
#include "src/part/ntfs_struct.hpp"
#include <array>
#include <config.h>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <new>
#include <print>
#include <ranges>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

auto ntfs_boot_sectors::scan(disk_t &disk, const partition_t &partition,
                             const int verbose) -> ntfs_boot_sectors
{
  ntfs_boot_sectors boot_sectors;
  log_info("\nntfs_boot_sector\n");
  log_partition(disk, partition);
  if (disk.pread(disk, boot_sectors.original.bootsector.data(),
                 NTFS_BOOT_SECTOR_SIZE,
                 partition.part_offset) != NTFS_BOOT_SECTOR_SIZE)
  {
    boot_sectors.original.bootsector_status =
        ntfs_boot_sectors::status::CANNOT_READ;
  }
  if (test_NTFS(disk,
                std::start_lifetime_as<struct ntfs_boot_sector>(
                    boot_sectors.original.bootsector.data()
                ),
                partition, verbose, 0) == 0)
  {
    boot_sectors.original.bootsector_status = ntfs_boot_sectors::status::OK;
  }
  else
  {
    boot_sectors.original.bootsector_status = ntfs_boot_sectors::status::BAD;
  }
  if (disk.pread(disk, boot_sectors.backup.bootsector.data(),
                 NTFS_BOOT_SECTOR_SIZE,
                 partition.part_offset + partition.part_size -
                     disk.sector_size) != NTFS_BOOT_SECTOR_SIZE)
  {
    boot_sectors.backup.bootsector_status =
        ntfs_boot_sectors::status::CANNOT_READ;
  }
  if (test_NTFS(disk,
                std::start_lifetime_as<struct ntfs_boot_sector>(
                    boot_sectors.backup.bootsector.data()
                ),
                partition, verbose, 0) == 0)
  {
    boot_sectors.backup.bootsector_status = ntfs_boot_sectors::status::OK;
  }
  else
  {
    boot_sectors.backup.bootsector_status = ntfs_boot_sectors::status::BAD;
  }
  if (boot_sectors.original.bootsector == boot_sectors.backup.bootsector)
  {
    log_ntfs_info(std::start_lifetime_as<struct ntfs_boot_sector>(
        boot_sectors.original.bootsector.data()
    ));
    boot_sectors.are_identical = true;
  }
  else
  {
    log_ntfs2_info(std::start_lifetime_as<struct ntfs_boot_sector>(
                       boot_sectors.original.bootsector.data()
                   ),
                   std::start_lifetime_as<struct ntfs_boot_sector>(
                       boot_sectors.backup.bootsector.data()
                   ));
    boot_sectors.are_identical = false;
  }

  return boot_sectors;
}

auto ntfs_boot_sectors::get_options(int *selected,
                                    const bool expert) const noexcept
    -> ntfs_boot_sectors::options
{
  if (backup.bootsector_status == status::OK &&
      original.bootsector_status == status::OK)
  {
    if (ntfs_boot_sectors::are_identical)
      return {
          .has_list     = true,
          .has_original = true,
          .has_backup   = true,
          .has_mft      = false,
      };
    return {
        .has_list     = true,
        .has_original = false,
        .has_backup   = false,
        .has_mft      = true,
    };
  }
  if (backup.bootsector_status == status::OK)
  {
    *selected = 5;
    if (expert)
      return {
          .has_list     = true,
          .has_original = false,
          .has_backup   = true,
          .has_mft      = true,
      };
    return {
        .has_list     = true,
        .has_original = false,
        .has_backup   = true,
        .has_mft      = false,
    };
  }
  if (original.bootsector_status == status::OK)
  {
    *selected = 4;
    return {
        .has_list     = true,
        .has_original = true,
        .has_backup   = false,
        .has_mft      = false,
    };
  }
  return {};
}

auto ntfs_boot_sectors::dump(const std::filesystem::path &path) const noexcept
    -> std::pair<std::string, std::string>
{
  std::pair<std::string, std::string> result {"SUCCESS", "SUCCESS"};
  std::ofstream original_dump(path / "bootsector.dump");
  if (original_dump.is_open())
  {
    log_info("dumping bootsector...");
    try
    {
      std::println(original_dump, "Boot sector");
      for (auto [i, ch] : original.bootsector | std::views::enumerate)
      {
        std::print(original_dump, "{:02X}", ch);
        if (i % 16 == 15)
          std::println(original_dump);
      }
    }
    catch (const std::bad_alloc &)
    {
      log_error("bad_alloc: run out of memory");
      result.first = "run out of memory";
    }
    catch (const std::system_error &e)
    {
      log_error("{} error: {}: {}", e.code().category().name(), e.what(),
                e.code().message());
      result.first = "writing failed";
    }
    catch (const std::exception &e)
    {
      log_error("unexpected exception: ", e.what());
      result.first = std::format("exeption thrown: {}", e.what());
    }
  }
  else
  {
    log_warning("failed creating {}", path / "original_bootsector.dump");
    result.first = "file creation failed";
  }

  std::ofstream backup_dump(path / "backup_bootsector.dump");
  if (backup_dump.is_open())
  {
    log_info("dumping backup bootsector...");
    try
    {
      std::println(backup_dump, "Backup boot sector");
      for (auto [i, ch] : original.bootsector | std::views::enumerate)
      {
        std::print(backup_dump, "{:02X}", ch);
        if (i % 16 == 15)
          std::println(backup_dump);
      }
    }
    catch (const std::bad_alloc &)
    {
      log_error("bad_alloc: run out of memory");
      result.second = "run out of memory";
    }
    catch (const std::system_error &e)
    {
      log_error("{} error: {}: {}", e.code().category().name(), e.what(),
                e.code().message());
      result.second = "writing failed";
    }
    catch (const std::exception &e)
    {
      log_error("unexpected exception: ", e.what());
      result.second = std::format("exeption thrown: {}", e.what());
    }
  }
  else
  {
    log_warning("failed creating {}", path / "backup_bootsector.dump");
    result.second = "file creation failed";
  }

  return result;
}

[[nodiscard]] auto ntfs_boot_sectors::raw_bootsector::
    status_text() const noexcept -> std::string_view
{
  switch (bootsector_status)
  {
    using enum status;
  case OK:
    return "OK";
  case BAD:
    return "BAD";
  case CANNOT_READ:
    return "Can't read backup boot sector.";
  default:
    std::unreachable();
  }
}
