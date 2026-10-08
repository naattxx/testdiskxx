/*

    File: ntfs_inc.h

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
#ifndef _NTFS_INC_H
#define _NTFS_INC_H

#include <config.h>

#if defined(HAVE_LIBNTFS) || defined(HAVE_LIBNTFS3G)
#include "src/common.hpp"
#include "src/dir_common.hpp"
#if __has_include(<ntfs/volume.h>)
#include <ntfs/volume.h>
#elif __has_include(<ntfs-3g/volume.h>)
#include <ntfs-3g/volume.h>
#endif
#undef min
#undef max
#ifdef HAVE_ICONV
#include <iconv.h>
#endif

struct ntfs_dir_struct : dir_data_t
{
  dir_list_t dir_list;
  ntfs_volume *vol;
  my_data_t *my_data;
  dir_data_t *dir_data;
  unsigned long int inode;
#ifdef HAVE_ICONV
  iconv_t cd;
#endif
  auto get_dir(disk_t &disk_car, const partition_t &partition,
                const unsigned long int first_inode, dir_list_t &list) -> int final;
  auto copy_file(disk_t &disk_car, const partition_t &partition,
                          const file_info_t &file) -> copy_file_t final;
  void close() final;
};
#endif

#endif
