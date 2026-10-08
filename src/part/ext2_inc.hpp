/*

    File: ext2_inc.h

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
#ifndef _EXT2_INC_H
#define _EXT2_INC_H

#include <config.h>

#ifdef DISABLED_FOR_FRAMAC
#undef HAVE_LIBEXT2FS
#endif

#ifdef HAVE_LIBEXT2FS
#include "src/common.hpp"
#include "src/dir_common.hpp"
#include <ext2fs/ext2fs.h>
struct ext2_dir_struct : dir_data_t
{
  dir_list_t dir_list;
  ext2_filsys current_fs;
  int flags;
  dir_data_t *dir_data;
  auto get_dir(disk_t &disk_car, const partition_t &partition,
                const unsigned long int first_inode, dir_list_t &list) -> int final;
  auto copy_file(disk_t &disk_car, const partition_t &partition,
                          const file_info_t &file) -> copy_file_t final;
  void close() final;
};
#endif

#endif
