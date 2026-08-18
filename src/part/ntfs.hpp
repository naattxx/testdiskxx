/*

    File: ntfs.h

    Copyright (C) 1998-2006,2008 Christophe GRENIER <grenier@cgsecurity.org>

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
#ifndef _NTFS_H
#define _NTFS_H
#include "ntfs_struct.hpp"
#include "src/common.hpp"

/*@
  @ requires \valid(disk_car);
  @ requires valid_disk(disk_car);
  @ requires \valid(partition);
  @ requires \separated(disk_car, partition);
  @ decreases 0;
  @*/
auto check_NTFS(disk_t &disk_car, partition_t &partition, const int verbose,
                const int dump_ind) -> int;

/*@
  @ requires \valid_read(ntfs_header);
  @*/
auto log_ntfs_info(const struct ntfs_boot_sector *ntfs_header) -> int;

/*@
  @ requires \valid_read(partition);
  @ assigns  \nothing;
  @*/
auto is_ntfs(const partition_t &partition) -> int;

/*@
  @ requires \valid_read(partition);
  @ assigns  \nothing;
  @*/
auto is_part_ntfs(const partition_t &partition) -> int;

/*@
  @ requires \valid(disk_car);
  @ requires valid_disk(disk_car);
  @ requires \valid_read(ntfs_header);
  @ requires \valid(partition);
  @ requires \separated(disk_car, ntfs_header, partition);
  @*/
auto recover_NTFS(disk_t &disk_car, const struct ntfs_boot_sector *ntfs_header,
                  partition_t &partition, const int verbose, const int dump_ind,
                  const int backup) -> int;

/*@
  @ requires \valid_read(disk_car);
  @ requires \valid_read(ntfs_header);
  @ requires \valid_read(partition);
  @ requires \separated(disk_car, ntfs_header, partition);
  @*/
auto test_NTFS(const disk_t &disk_car,
               const struct ntfs_boot_sector *ntfs_header,
               const partition_t &partition, const int verbose,
               const int dump_ind) -> int;

#define NTFS_GETU8(p) (*(const uint8_t *)(p))
#define NTFS_GETU16(p) (le16(*(const uint16_t *)(p)))
#define NTFS_GETU32(p) (le32(*(const uint32_t *)(p)))
#define NTFS_GETU64(p) (le64(*(const uint64_t *)(p)))

/*@
  @ requires \valid_read(ntfs_header);
  @ assigns  \nothing;
  @*/
auto ntfs_sector_size(const struct ntfs_boot_sector *ntfs_header)
    -> unsigned int;

/*@
  @ requires \valid_read(record);
  @*/
auto ntfs_findattribute(const ntfs_recordheader *record, uint32_t attrType,
                        const char *end) -> const ntfs_attribheader *;

/*@
  @ requires \valid_read(attrib);
  @ assigns  \nothing;
  @*/
auto ntfs_getattributedata(const ntfs_attribresident *attrib, const char *end)
    -> const char *;

/*@
  @ requires \valid_read(attrnr);
  @*/
auto ntfs_get_first_rl_element(const ntfs_attribnonresident *attrnr,
                               const char *end) -> long int;

#endif
