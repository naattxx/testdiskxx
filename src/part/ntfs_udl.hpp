/*

    File: ntfs_udl.h

    Copyright (C) 2007 Christophe GRENIER <grenier@cgsecurity.org>

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
#ifndef _NTFS_UDL_H
#define _NTFS_UDL_H
#include "src/common.hpp"
#include "src/dir_common.hpp"
#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <utility>
#ifdef HAVE_LIBNTFS
#include <ntfs/volume.h>
#elifdef HAVE_LIBNTFS3G
extern "C"
{
#include <ntfs-3g/volume.h>
}
#undef min
#undef max
#endif

/*@
  @ requires \valid(disk_car);
  @ requires valid_disk(disk_car);
  @ requires \valid_read(partition);
  @ requires \separated(disk_car, partition);
  @*/
auto ntfs_undelete_part_data(disk_t &disk_car, const partition_t &partition,
                             const int verbose)
    -> std::expected<std::pair<dir_data_t *, dir_list_t>, std::string>;

#if defined(HAVE_LIBNTFS) || defined(HAVE_LIBNTFS3G)

auto undelete_file(ntfs_volume *vol, uint64_t inode, const std::filesystem::path &dest) -> int;

#endif

#endif
