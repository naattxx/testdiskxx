/*

    File: parthumax.c

    Copyright (C) 2010 Christophe GRENIER <grenier@cgsecurity.org>

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

#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_HUMAX)
#include <config.h>

#include <cassert>
#include <cctype> /* tolower */
#include <cstdio>
#include <cstdlib>
#include <cstring>
// #include "types.h"
#include "parthumax.hpp"
#include "src/chgtype.hpp"
#include "src/common.hpp"
#include "src/fnctdsk.hpp"
#include "src/intrf.hpp"
#include "src/lang.h"
#include "src/log.hpp"

/*@
  @ requires \valid(disk_car);
  @ requires valid_disk(disk_car);
  @*/
// ensures  valid_list_part(\result);
static auto read_part_humax(disk_t &disk_car, const int verbose,
                            const int saveheader) -> list_part_t;

/*@
  @ requires \valid_read(disk_car);
  @ requires valid_disk(disk_car);
  @ requires \valid(list_part);
  @ requires separation: \separated(disk_car, list_part);
  @*/
static auto write_part_humax(disk_t &disk_car, const list_part_t &list_part,
                             const int ro, const int verbose) -> int;

/*@
  @ requires \valid(disk_car);
  @ requires list_part == \null || \valid(list_part);
  @ requires separation: \separated(disk_car, list_part);
  @*/
static void init_part_order_humax(const disk_t &disk_car,
                                  list_part_t &list_part);

/*@
  @ requires \valid_read(disk_car);
  @ requires \valid(partition);
  @ requires separation: \separated(disk_car, partition);
  @ assigns partition.status;
  @*/
static void set_next_status_humax(const disk_t &disk_car,
                                  partition_t &partition);

/*@
  @ requires list_part == \null || \valid_read(list_part);
  @*/
static auto test_structure_humax(const list_part_t &list_part) -> int;

/*@
  @ requires \valid(partition);
  @ assigns \nothing;
  @*/
static auto is_part_known_humax(const partition_t &partition) -> int;

/*@
  @ requires \valid_read(disk_car);
  @ requires list_part == \null || \valid(list_part);
  @*/
static void init_structure_humax(const disk_t &disk_car, list_part_t &list_part,
                                 const int verbose);

/*@
  @ requires \valid_read(partition);
  @ assigns \nothing;
  @*/
static auto get_partition_typename_humax(const partition_t &partition) -> const
    char *;

/*@
  @ requires \valid_read(partition);
  @ assigns \nothing;
  @*/
static auto get_part_type_humax(const partition_t &partition) -> unsigned int;

#if 0
static const struct systypes humax_sys_types[] = {
  {0x00,	 	"Empty"        	},
  {PHUMAX_PARTITION,	"Partition"	},
  {0, NULL }
};
#endif

struct [[gnu::gcc_struct, gnu::packed]] partition_humax
{
  uint32_t unk1;
  uint32_t num_sectors;
  uint32_t unk2;
  uint32_t start_sector;
};

struct [[gnu::gcc_struct, gnu::packed]] humaxlabel
{
  char unk1[0x1be];
  struct partition_humax partitions[4];
  uint16_t magic;
};

arch_fnct_t arch_humax = {
    .part_name              = "Humax",
    .part_name_option       = "partition_humax",
    .msg_part_type          = "                P=Primary  D=Deleted",
    .read_part              = &read_part_humax,
    .write_part             = &write_part_humax,
    .init_part_order        = &init_part_order_humax,
    .get_geometry_from_mbr  = nullptr,
    .check_part             = nullptr,
    .write_MBR_code         = nullptr,
    .set_prev_status        = &set_next_status_humax,
    .set_next_status        = &set_next_status_humax,
    .test_structure         = &test_structure_humax,
    .get_part_type          = &get_part_type_humax,
    .set_part_type          = nullptr,
    .init_structure         = &init_structure_humax,
    .erase_list_part        = nullptr,
    .get_partition_typename = &get_partition_typename_humax,
    .is_part_known          = &is_part_known_humax};

static auto is_part_known_humax(const partition_t &partition) -> int
{
  return (partition.part_type_humax != PHUMAX_PARTITION);
}

static auto get_part_type_humax(const partition_t &partition) -> unsigned int
{
  return partition.part_type_humax;
}

static auto read_part_humax(disk_t &disk_car, const int verbose,
                            const int saveheader) -> list_part_t
{
  unsigned int i;
  struct humaxlabel *humaxlabel;
  list_part_t new_list_part;
  uint32_t *p32;
  unsigned char *buffer;
  /*@ assert valid_list_part(new_list_part); */
  if (disk_car.sector_size < DEFAULT_SECTOR_SIZE)
    return new_list_part;
  buffer = new unsigned char[disk_car.sector_size];
  screen_buffer_reset();
  humaxlabel = reinterpret_cast<struct humaxlabel *>(buffer);
  p32        = reinterpret_cast<uint32_t *>(buffer);
  if (disk_car.pread(disk_car, buffer, DEFAULT_SECTOR_SIZE,
                     static_cast<uint64_t>(0)) != DEFAULT_SECTOR_SIZE)
  {
    screen_buffer_add(msg_PART_RD_ERR);
    delete[] (buffer);
    return new_list_part;
  }
  for (i = 0; i < 0x200 / 4; i++)
    p32[i] = be32(p32[i]);
  ; // dump_log(buffer, DEFAULT_SECTOR_SIZE);
  if (le16(humaxlabel->magic) != 0xAA55)
  {
    screen_buffer_add("Bad HUMAX partition\n");
    delete[] (buffer);
    return new_list_part;
  }
  /*@
    @ loop invariant valid_list_part(new_list_part);
    @*/
  for (i = 0; i < 4; i++)
  {
    if (humaxlabel->partitions[i].num_sectors > 0)
    {
      int insert_error = 0;
      partition_t new_partition(&arch_humax);
      new_partition.order           = i + 1;
      new_partition.part_type_humax = PHUMAX_PARTITION;
      new_partition.part_offset =
          be32(humaxlabel->partitions[i].start_sector) * disk_car.sector_size;
      new_partition.part_size =
          static_cast<uint64_t>(be32(humaxlabel->partitions[i].num_sectors)) *
          disk_car.sector_size;
      new_partition.status = STATUS_PRIM;
      //       disk_car.arch->check_part(disk_car,verbose,new_partition,saveheader);
      aff_part_buffer(AFF_PART_ORDER | AFF_PART_STATUS, disk_car,
                      new_partition);
      insert_new_partition(new_list_part, new_partition, 0, &insert_error);
    }
  }
  delete[] (buffer);
  return new_list_part;
}

static auto write_part_humax(disk_t &disk_car, const list_part_t &list_part,
                             const int ro, const int verbose) -> int
{
  /* TODO: Implement it */
  if (ro == 0)
    return -1;
  return 0;
}

static void init_part_order_humax(const disk_t &disk_car,
                                  list_part_t &list_part)
{
  int nbr_prim = 0;
  for (partition_t &element : list_part)
  {
    switch (element.status)
    {
    case STATUS_PRIM:
      element.order = nbr_prim++;
      break;
    default:
      log_critical("init_part_order_humax: severe error\n");
      break;
    }
  }
}

void add_partition_humax_cli(const disk_t &disk_car, list_part_t &list_part,
                             char **current_cmd)
{
  CHS_t start, end;
  partition_t new_partition(&arch_humax);
  assert(current_cmd != nullptr);
  start.cylinder = 0;
  start.head     = 0;
  start.sector   = 1;
  end.cylinder   = disk_car.geom.cylinders - 1;
  end.head       = disk_car.geom.heads_per_cylinder - 1;
  end.sector     = disk_car.geom.sectors_per_head;
  /*@
    @ loop invariant valid_list_part(list_part);
    @ loop invariant valid_read_string(*current_cmd);
    @ */
  while (true)
  {
    skip_comma_in_command(current_cmd);
    /*@ assert valid_read_string(*current_cmd); */
    if (check_command(current_cmd, "c,", 2) == 0)
    {
      start.cylinder = ask_number_cli(current_cmd, start.cylinder, 0,
                                      disk_car.geom.cylinders - 1,
                                      "Enter the starting cylinder ");
    }
    else if (check_command(current_cmd, "C,", 2) == 0)
    {
      end.cylinder = ask_number_cli(current_cmd, end.cylinder, start.cylinder,
                                    disk_car.geom.cylinders - 1,
                                    "Enter the ending cylinder ");
    }
    else if (check_command(current_cmd, "T,", 2) == 0)
    {
      change_part_type_cli(disk_car, new_partition, current_cmd);
    }
    else if ((CHS2offset(disk_car, &end) > new_partition.part_offset) &&
             new_partition.part_type_humax > 0)
    {
      int insert_error = 0;
      insert_new_partition(list_part, new_partition, 0, &insert_error);
      /*@ assert valid_list_part(new_list_part); */
      if (insert_error > 0)
      {
        /*@ assert valid_list_part(new_list_part); */
        return;
      }
      new_partition.status = STATUS_PRIM;
      if (test_structure_humax(list_part) != 0)
        new_partition.status = STATUS_DELETED;
      /*@ assert valid_read_string(*current_cmd); */
      /*@ assert valid_list_part(new_list_part); */
      return;
    }
    /*@ assert valid_read_string(*current_cmd); */
    /*@ assert valid_list_part(list_part); */
  }
}

static void set_next_status_humax(const disk_t &disk_car,
                                  partition_t &partition)
{
  if (partition.status == STATUS_DELETED)
    partition.status = STATUS_PRIM;
  else
    partition.status = STATUS_DELETED;
}

static auto test_structure_humax(const list_part_t &list_part) -> int
{ /* Return 1 if bad*/
  list_part_t new_list_part;
  int res;
  unsigned int nbr_prim = 0;
  /*@ loop assigns element, nbr_prim; */
  for (const partition_t &element : list_part)
  {
    if (element.status == STATUS_PRIM)
      nbr_prim++;
  }
  if (nbr_prim > 4)
    return 1;
  new_list_part = gen_sorted_partition_list(list_part);
  res           = is_part_overlapping(new_list_part);
  return res;
}

static void init_structure_humax(const disk_t &disk_car, list_part_t &list_part,
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
  if (test_structure_humax(new_list_part))
  {
    for (partition_t &element : new_list_part)
      element.status = STATUS_DELETED;
  }
  list_part = new_list_part;
}

static auto get_partition_typename_humax(const partition_t &partition) -> const
    char *
{
  return "Partition";
}
#endif
