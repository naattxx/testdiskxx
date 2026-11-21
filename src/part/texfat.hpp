/*

    File: texfat.h

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
#ifndef _TEXFAT_H
#define _TEXFAT_H
#include "src/common.hpp"
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class exfat_boot_sectors
{
  enum class status : uint8_t
  {
    OK = 0,
    BAD,
    CANNOT_READ,
  };
  class raw_bootsector
  {
    friend exfat_boot_sectors;

  private:
    status bootsector_status;

  public:
    std::vector<unsigned char> bootsector;
    [[nodiscard]] auto status_text() const noexcept -> std::string_view;
  };

public:
  struct [[nodiscard]] options
  {
    bool has_original, has_backup;
  };
  bool are_identical;
  raw_bootsector original, backup;

  auto get_options() const noexcept -> options;
  [[nodiscard]] auto dump(const std::filesystem::path &path) const noexcept
      -> std::pair<std::string, std::string>;
  static auto scan(disk_t &disk, const partition_t &partition)
      -> exfat_boot_sectors;
};

#endif
