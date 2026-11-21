/*

    File: texFAT.c

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

#include "exfat.hpp"
#include "src/common.hpp"
#include "src/log.hpp"
#include "src/log_part.hpp"
#include "texfat.hpp"
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

auto exfat_boot_sectors::dump(const std::filesystem::path &path) const noexcept
    -> std::pair<std::string, std::string>
{
  constexpr const char *original_bootsector_file_name =
      "original_superblock.dump";
  constexpr const char *backup_bootsector_file_name = "backup_superblock.dump";
  std::pair<std::string, std::string> result {"SUCCESS", "SUCCESS"};
  std::ofstream original_dump(path / original_bootsector_file_name);
  if (original_dump.is_open())
  {
    log_info("dumping superblock...");
    try
    {
      std::println(original_dump, "Superblock");
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
    log_warning("failed creating {}", path / original_bootsector_file_name);
    result.first = "file creation failed";
  }

  std::ofstream backup_dump(path / backup_bootsector_file_name);
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
    log_warning("failed creating {}", path / backup_bootsector_file_name);
    result.second = "file creation failed";
  }

  return result;
}

auto exfat_boot_sectors::get_options() const noexcept -> options
{
  if (are_identical)
    return {.has_original = false, .has_backup = false};
  return {
      .has_original = original.bootsector_status == status::OK,
      .has_backup   = backup.bootsector_status == status::OK,
  };
}

auto exfat_boot_sectors::scan(disk_t &disk, const partition_t &partition)
    -> exfat_boot_sectors
{
  const int bootsector_size = 12 * disk.sector_size;
  exfat_boot_sectors boot_sectors;
  log_info("\nexFAT_boot_sector\n");
  log_partition(disk, partition);

  boot_sectors.original.bootsector.resize(bootsector_size);
  if (disk.pread(disk, boot_sectors.original.bootsector.data(), bootsector_size,
                 partition.part_offset) != bootsector_size)
  {
    boot_sectors.original.bootsector_status =
        exfat_boot_sectors::status::CANNOT_READ;
  }
  else if (test_exFAT(std::start_lifetime_as<struct exfat_super_block>(
               boot_sectors.original.bootsector.data()
           )) == 0)
  {
    boot_sectors.original.bootsector_status = exfat_boot_sectors::status::OK;
  }
  else
  {
    boot_sectors.original.bootsector_status = exfat_boot_sectors::status::BAD;
  }

  boot_sectors.backup.bootsector.resize(bootsector_size);
  if (disk.pread(disk, boot_sectors.backup.bootsector.data(), bootsector_size,
                 partition.part_offset + bootsector_size) != bootsector_size)
  {
    boot_sectors.backup.bootsector_status =
        exfat_boot_sectors::status::CANNOT_READ;
  }
  else if (test_exFAT(std::start_lifetime_as<struct exfat_super_block>(
               boot_sectors.backup.bootsector.data()
           )) == 0)
  {
    boot_sectors.backup.bootsector_status = exfat_boot_sectors::status::OK;
  }
  else
  {
    boot_sectors.backup.bootsector_status = exfat_boot_sectors::status::BAD;
  }

  boot_sectors.are_identical =
      boot_sectors.original.bootsector == boot_sectors.backup.bootsector;

  return boot_sectors;
}
[[nodiscard]] auto exfat_boot_sectors::raw_bootsector::
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
    return "Can't read boot record.";
  default:
    std::unreachable();
  }
}
