/*

    File: ntfs_fix.h

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
#ifndef _NTFS_FIX_H
#define _NTFS_FIX_H
#include "src/common.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string_view>

class mft_state
{
private:
  disk_t &disk;
  std::span<std::byte> buffer_mft;
  std::span<std::byte> buffer_mftmirr;
  uint64_t mft_pos;
  uint64_t mftmirr_pos;

public:
  bool good_mft, good_mft_mirror;

  explicit mft_state(disk_t &disk, std::span<std::byte> buffer_mft,
                     std::span<std::byte> buffer_mftmirr, uint64_t mft_pos,
                     uint64_t mftmirr_pos, bool is_mft_good,
                     bool is_mftmirr_good)
      : disk(disk), buffer_mft(buffer_mft), buffer_mftmirr(buffer_mftmirr),
        mft_pos(mft_pos), mftmirr_pos(mftmirr_pos), good_mft(is_mft_good),
        good_mft_mirror(is_mftmirr_good) {};

  ~mft_state()
  {
    delete buffer_mft.data();
    delete buffer_mftmirr.data();
  }

  [[nodiscard]] auto fix_mft() -> std::string_view;
  [[nodiscard]] auto fix_mft_mirror() -> std::string_view;

  /*@
    @ requires \valid(disk_car);
    @ requires valid_disk(disk_car);
    @ requires \valid(partition);
    @ requires \separated(disk_car, partition);
    @*/
  [[nodiscard]]
  static auto get(disk_t &disk, partition_t &partition, const int verbose,
                  const bool expert)
      -> std::expected<mft_state, std::string_view>;
};

#endif
