/*

    File: partxbox.c

    Copyright (C) 2005-2008 Christophe GRENIER <grenier@cgsecurity.org>

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

#include <string_view>
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_XBOX)
#include <config.h>

#include <cassert>
#include <cctype> /* tolower */
#include <cstdio>
#include <cstdlib>
#include <cstring>
// #include "types.h"
#include "fatx.hpp"
#include "partxbox.hpp"
#include "src/chgtype.hpp"
#include "src/common.hpp"
#include "src/fnctdsk.hpp"
#include "src/intrf.hpp"
#include "src/lang.h"
#include "src/log.hpp"
#include "src/savehdr.hpp"

/*@
  @ requires \valid(disk_car);
  @ requires \valid(partition);
  @*/
static auto check_part_xbox(disk_t &disk_car, const int verbose,
                            partition_t &partition, const int saveheader)
    -> int;

/*@
  @ requires \valid(disk_car);
  @ requires valid_disk(disk_car);
  @*/
// ensures  valid_list_part(\result);
static auto read_part_xbox(disk_t &disk_car, const int verbose,
                           const int saveheader) -> list_part_t;

/*@
  @ requires \valid_read(disk_car);
  @ requires \valid(list_part);
  @ requires separation: \separated(disk_car, list_part);
  @*/
static auto write_part_xbox(disk_t &disk_car, const list_part_t &list_part,
                            const int ro, const int verbose) -> int;

/*@
  @ requires \valid(disk_car);
  @ requires list_part == \null || \valid(list_part);
  @ requires separation: \separated(disk_car, list_part);
  @*/
static void init_part_order_xbox(const disk_t &disk_car,
                                 list_part_t &list_part);

/*@
  @ requires \valid_read(disk_car);
  @ requires \valid(partition);
  @ requires separation: \separated(disk_car, partition);
  @ assigns partition.status;
  @*/
static void set_next_status_xbox(const disk_t &disk_car,
                                 partition_t &partition);

/*@
  @ requires list_part == \null || \valid_read(list_part);
  @*/
static auto test_structure_xbox(const list_part_t &list_part) -> int;

/*@
  @ requires \valid(partition);
  @ assigns partition.part_type_xbox;
  @*/
static auto set_part_type_xbox(partition_t &partition,
                               unsigned int part_type_xbox) -> int;

/*@
  @ requires \valid(partition);
  @ assigns \nothing;
  @*/
static auto is_part_known_xbox(const partition_t &partition) -> int;

/*@
  @ requires \valid_read(disk_car);
  @ requires list_part == \null || \valid(list_part);
  @*/
static void init_structure_xbox(const disk_t &disk_car, list_part_t &list_part,
                                const int verbose);

/*@
  @ requires \valid_read(partition);
  @ assigns \nothing;
  @*/
static auto get_partition_typename_xbox(const partition_t &partition) -> std::string_view;

/*@
  @ assigns \nothing;
  @*/
static auto get_partition_typename_xbox_aux(const unsigned int part_type_xbox)
    -> std::string_view;

/*@
  @ requires \valid_read(partition);
  @ assigns \nothing;
  @*/
static auto get_part_type_xbox(const partition_t &partition) -> unsigned int;

static const struct systypes xbox_sys_types[] = {
    {.part_type = PXBOX_UNK,  .name = "Unknown"},
    {.part_type = PXBOX_FATX, .name = "FATX"   },
};

arch_fnct_t arch_xbox = {.part_name        = "XBox",
                         .part_name_option = "partition_xbox",
                         .msg_part_type =
                             "                P=Primary  D=Deleted",
                         .read_part              = &read_part_xbox,
                         .write_part             = &write_part_xbox,
                         .init_part_order        = &init_part_order_xbox,
                         .get_geometry_from_mbr  = nullptr,
                         .check_part             = &check_part_xbox,
                         .write_MBR_code         = nullptr,
                         .set_prev_status        = &set_next_status_xbox,
                         .set_next_status        = &set_next_status_xbox,
                         .test_structure         = &test_structure_xbox,
                         .get_part_type          = &get_part_type_xbox,
                         .set_part_type          = &set_part_type_xbox,
                         .init_structure         = &init_structure_xbox,
                         .erase_list_part        = nullptr,
                         .get_partition_typename = &get_partition_typename_xbox,
                         .is_part_known          = &is_part_known_xbox};

static auto get_part_type_xbox(const partition_t &partition) -> unsigned int
{
  return partition.part_type_xbox;
}

static auto read_part_xbox(disk_t &disk_car, const int verbose,
                           const int saveheader) -> list_part_t
{
  unsigned char buffer[0x800];
  list_part_t new_list_part;
  /*@ assert valid_list_part(new_list_part); */
  screen_buffer_reset();
  if (disk_car.pread(disk_car, &buffer, sizeof(buffer), 0) != sizeof(buffer))
    return new_list_part;
  {
    uint64_t offsets[] = {0x00080000, 0x2ee80000, 0x5dc80000, 0x8ca80000,
                          0xabe80000};
    unsigned int i;
    auto *xboxlabel = reinterpret_cast<struct xbox_partition *>(&buffer);
    if (memcmp(xboxlabel->magic, "BRFR", 4))
    {
      screen_buffer_add("\nBad XBOX partition, invalid signature\n");
      return new_list_part;
    }
    /*@
      @ loop invariant valid_list_part(new_list_part);
      @*/
    for (i = 0; i < sizeof(offsets) / sizeof(uint64_t); i++)
    {
      if (offsets[i] < disk_car.disk_size)
      {
        int _insert_error = 0;
        partition_t partition(&arch_xbox);
        partition.part_type_xbox = PXBOX_FATX;
        partition.part_offset    = offsets[i];
        partition.order          = 1 + i;
        if (i == sizeof(offsets) / sizeof(uint64_t) - 1 ||
            disk_car.disk_size <= offsets[i + 1])
          partition.part_size = disk_car.disk_size - offsets[i];
        else
          partition.part_size = offsets[i + 1] - offsets[i];
        partition.status = STATUS_PRIM;
        check_part_xbox(disk_car, verbose, partition, saveheader);
        aff_part_buffer(AFF_PART_ORDER | AFF_PART_STATUS, disk_car, partition);
        insert_new_partition(new_list_part, partition, 0, &_insert_error);
      }
    }
  }
  /*@ assert valid_list_part(new_list_part); */
  return new_list_part;
}

static auto write_part_xbox(disk_t &disk_car, const list_part_t &list_part,
                            const int ro, const int verbose) -> int
{
  /* TODO: Implement it */
  if (ro == 0)
    return -1;
  return 0;
}

static void init_part_order_xbox(const disk_t &disk_car, list_part_t &list_part)
{
  ;
}

void add_partition_xbox_cli(const disk_t &disk_car, list_part_t &list_part,
                            char **current_cmd)
{
  partition_t new_partition(&arch_xbox);
  assert(current_cmd != nullptr);
  new_partition.part_offset = disk_car.sector_size;
  new_partition.part_size   = disk_car.disk_size - disk_car.sector_size;
  /*@
    @ loop invariant valid_list_part(list_part);
    @ loop invariant valid_read_string(*current_cmd);
    @ */
  while (true)
  {
    skip_comma_in_command(current_cmd);
    if (check_command(current_cmd, "s,", 2) == 0)
    {
      uint64_t part_offset;
      part_offset = new_partition.part_offset;
      new_partition.part_offset =
          ask_number_cli(current_cmd,
                         new_partition.part_offset / disk_car.sector_size,
                         0x800 / disk_car.sector_size,
                         (disk_car.disk_size - 1) / disk_car.sector_size,
                         "Enter the starting sector ") *
          static_cast<uint64_t>(disk_car.sector_size);
      new_partition.part_size =
          new_partition.part_size + part_offset - new_partition.part_offset;
    }
    else if (check_command(current_cmd, "S,", 2) == 0)
    {
      new_partition.part_size =
          ask_number_cli(
              current_cmd,
              (new_partition.part_offset + new_partition.part_size - 1) /
                  disk_car.sector_size,
              new_partition.part_offset / disk_car.sector_size,
              (disk_car.disk_size - 1) / disk_car.sector_size,
              "Enter the ending sector "
          ) * static_cast<uint64_t>(disk_car.sector_size) +
          disk_car.sector_size - new_partition.part_offset;
    }
    else if (check_command(current_cmd, "T,", 2) == 0)
    {
      change_part_type_cli(disk_car, new_partition, current_cmd);
    }
    else if (new_partition.part_size > 0 && new_partition.part_type_xbox > 0)
    {
      int insert_error = 0;
      insert_new_partition(list_part, new_partition, 0, &insert_error);
      /*@ assert valid_list_part(list_part); */
      if (insert_error > 0)
      {
        /*@ assert valid_list_part(list_part); */
        return;
      }
      new_partition.status = STATUS_PRIM;
      if (test_structure_xbox(list_part) != 0)
        new_partition.status = STATUS_DELETED;
      /*@ assert valid_list_part(list_part); */
      return;
    }
    /*@ assert valid_list_part(list_part); */
  }
}

static void set_next_status_xbox(const disk_t &disk_car, partition_t &partition)
{
  if (partition.status == STATUS_DELETED)
    partition.status = STATUS_PRIM;
  else
    partition.status = STATUS_DELETED;
}

static auto test_structure_xbox(const list_part_t &list_part) -> int
{ /* Return 1 if bad*/
  list_part_t new_list_part;
  int res;
  new_list_part = gen_sorted_partition_list(list_part);
  res           = is_part_overlapping(new_list_part);
  return res;
}

static auto set_part_type_xbox(partition_t &partition,
                               unsigned int part_type_xbox) -> int
{
  if (part_type_xbox > 0 && part_type_xbox <= 255)
  {
    partition.part_type_xbox = part_type_xbox;
    return 0;
  }
  return 1;
}

static auto is_part_known_xbox(const partition_t &partition) -> int
{
  return (partition.part_type_xbox != PXBOX_UNK);
}

static void init_structure_xbox(const disk_t &disk_car, list_part_t &list_part,
                                const int verbose)
{
  list_part_t new_list_part;
  /* Create new list */
  for (partition_t &element : list_part)
    element.to_be_removed = 0;
  for (auto element = list_part.begin(); element != list_part.end();
       element      = std::next(element))
  {
    int insert_error = 0;
    for (auto element2 = std::next(element); element2 != list_part.end();
         element2      = std::next(element2))
    {
      if (element->part_offset + element->part_size - 1 >=
          element2->part_offset)
      {
        element->to_be_removed  = 1;
        element2->to_be_removed = 1;
      }
    }
    if (element->to_be_removed == 0)
      insert_new_partition(new_list_part, *element, 0, &insert_error);
  }
  for (partition_t &element : new_list_part)
    element.status = STATUS_PRIM;
  if (test_structure_xbox(new_list_part))
  {
    for (partition_t &element : new_list_part)
      element.status = STATUS_DELETED;
  }
  list_part = new_list_part;
}

static auto check_part_xbox(disk_t &disk_car, const int verbose,
                            partition_t &partition, const int saveheader) -> int
{
  int ret = 0;
  switch (partition.part_type_xbox)
  {
  case PXBOX_FATX:
    ret = check_FATX(disk_car, partition);
    if (ret != 0)
    {
      screen_buffer_add("Invalid FATX signature\n");
    }
    break;
  default:
    if (verbose > 0)
    {
      log_info("check_part_xbox {} type %02X: no test\n", partition.order,
               partition.part_type_xbox);
    }
    break;
  }
  if (ret != 0)
  {
    log_error("check_part_xbox failed for partition type %02X\n",
              partition.part_type_xbox);
    aff_part_buffer(AFF_PART_ORDER | AFF_PART_STATUS, disk_car, partition);
    if (saveheader > 0)
    {
      save_header(disk_car, partition, verbose);
    }
  }
  return ret;
}

static auto get_partition_typename_xbox_aux(const unsigned int part_type_xbox)
    -> std::string_view
{
  for (const auto &xbox_sys_type : xbox_sys_types)
    if (xbox_sys_type.part_type == part_type_xbox)
      return xbox_sys_type.name;
  return "";
}

static auto get_partition_typename_xbox(const partition_t &partition) -> std::string_view
{
  return get_partition_typename_xbox_aux(partition.part_type_xbox);
}
#endif
