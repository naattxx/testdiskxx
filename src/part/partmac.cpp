/*

    File: partmac.c

    Copyright (C) 1998-2008 Christophe GRENIER <grenier@cgsecurity.org>

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
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_MAC)
#include <config.h>

#include <cassert>
#include <cctype> /* tolower */
#include <cstdio>
#include <cstdlib>
#include <cstring>
// #include "types.h"
#include "src/common.hpp"
#include "src/fnctdsk.hpp"
#include "src/intrf.hpp"
#include "src/lang.h"
#ifndef DISABLED_FOR_FRAMAC
#include "src/analyse.hpp"
#endif
#include "fat.hpp"
#include "hfs.hpp"
#include "hfsp.hpp"
#include "partmac.hpp"
#include "src/log.hpp"
#include "src/savehdr.hpp"

/*@
  @ requires \valid(disk_car);
  @ requires \valid(partition);
  @*/
static auto check_part_mac(disk_t &disk_car, const int verbose,
                           partition_t &partition, const int saveheader) -> int;

/*@
  @ requires \valid(disk_car);
  @ requires valid_disk(disk_car);
  @ ensures  valid_list_part(\result);
  @*/
static auto read_part_mac(disk_t &disk_car, const int verbose,
                          const int saveheader) -> list_part_t;

/*@
  @ requires \valid(disk_car);
  @ requires list_part == \null || \valid(list_part);
  @*/
static auto write_part_mac(disk_t &disk_car, const list_part_t &list_part,
                           const int ro, const int verbose) -> int;

/*@
  @ requires \valid(disk_car);
  @ requires list_part == \null || \valid(list_part);
  @ requires separation: \separated(disk_car, list_part);
  @ assigns \nothing;
  @*/
static void init_part_order_mac(const disk_t &disk_car, list_part_t &list_part);

/*@
  @ requires \valid_read(disk_car);
  @ requires \valid(partition);
  @ requires separation: \separated(disk_car, partition);
  @ assigns partition.status;
  @*/
static void set_next_status_mac(const disk_t &disk_car, partition_t &partition);

/*@
  @ requires \valid(partition);
  @ assigns partition.part_type_mac;
  @*/
static auto set_part_type_mac(partition_t &partition,
                              unsigned int part_type_mac) -> int;

/*@
  @ requires \valid(partition);
  @ assigns \nothing;
  @*/
static auto is_part_known_mac(const partition_t &partition) -> int;

/*@
  @ requires \valid_read(disk_car);
  @ requires list_part == \null || \valid(list_part);
  @*/
static void init_structure_mac(const disk_t &disk_car, list_part_t &list_part,
                               const int verbose);

/*@
  @ requires \valid_read(partition);
  @ assigns \nothing;
  @*/
static auto get_partition_typename_mac(const partition_t &partition) -> std::string_view;

/*@
  @ assigns \nothing;
  @*/
static auto get_partition_typename_mac_aux(const unsigned int part_type_mac)
    -> std::string_view;

/*@
  @ requires \valid_read(partition);
  @ assigns \nothing;
  @*/
static auto get_part_type_mac(const partition_t &partition) -> unsigned int;

static const struct systypes mac_sys_types[] = {
    {.part_type = PMAC_DRIVER43,  .name = "Driver43"     },
    {.part_type = PMAC_DRIVERATA, .name = "Driver_ATA"   },
    {.part_type = PMAC_DRIVERIO,  .name = "Driver_IOKit" },
    {.part_type = PMAC_FREE,      .name = "Free"         },
    {.part_type = PMAC_FWDRIVER,  .name = "FWDriver"     },
    {.part_type = PMAC_SWAP,      .name = "Swap"         },
    {.part_type = PMAC_LINUX,     .name = "Linux"        },
    {.part_type = PMAC_BEOS,      .name = "BeFS"         },
    {.part_type = PMAC_HFS,       .name = "HFS"          },
    {.part_type = PMAC_MAP,       .name = "partition_map"},
    {.part_type = PMAC_PATCHES,   .name = "Patches"      },
    {.part_type = PMAC_UNK,       .name = "Unknown"      },
    {.part_type = PMAC_NewWorld,  .name = "NewWorld"     },
    {.part_type = PMAC_DRIVER,    .name = "Driver"       },
    {.part_type = PMAC_MFS,       .name = "MFS"          },
    {.part_type = PMAC_PRODOS,    .name = "ProDOS"       },
    {.part_type = PMAC_FAT32,     .name = "DOS_FAT_32"   },
};

arch_fnct_t arch_mac = {.part_name        = "Mac",
                        .part_name_option = "partition_mac",
                        .msg_part_type = "                P=Primary  D=Deleted",
                        .read_part     = &read_part_mac,
                        .write_part    = &write_part_mac,
                        .init_part_order        = &init_part_order_mac,
                        .get_geometry_from_mbr  = nullptr,
                        .check_part             = &check_part_mac,
                        .write_MBR_code         = nullptr,
                        .set_prev_status        = &set_next_status_mac,
                        .set_next_status        = &set_next_status_mac,
                        .test_structure         = &test_structure_mac,
                        .get_part_type          = &get_part_type_mac,
                        .set_part_type          = &set_part_type_mac,
                        .init_structure         = &init_structure_mac,
                        .erase_list_part        = nullptr,
                        .get_partition_typename = &get_partition_typename_mac,
                        .is_part_known          = &is_part_known_mac};

static auto get_part_type_mac(const partition_t &partition) -> unsigned int
{
  return partition.part_type_mac;
}

static auto read_part_mac(disk_t &disk_car, const int verbose,
                          const int saveheader) -> list_part_t
{
  unsigned char buffer[DEFAULT_SECTOR_SIZE];
  list_part_t new_list_part;
  unsigned int i;
  unsigned int limit = 1;
  screen_buffer_reset();
  if (disk_car.pread(disk_car, &buffer, sizeof(buffer), 0) != sizeof(buffer))
    return new_list_part;
  {
    auto *maclabel = reinterpret_cast<mac_Block0 *>(&buffer);
    if (be16(maclabel->sbSig) != BLOCK0_SIGNATURE)
    {
      screen_buffer_add("Bad MAC partition, invalid block0 signature\n");
      /* continue, even if the first sector have been overwritten by an Intel
     partition, the following sectors may be intact */
    }
  }
  for (i = 1; i <= limit; i++)
  {
    const auto *dpme = reinterpret_cast<const mac_DPME *>(buffer);
    if (disk_car.pread(disk_car, &buffer, sizeof(buffer),
                       static_cast<uint64_t>(i) * PBLOCK_SIZE) !=
        sizeof(buffer))
      return new_list_part;
    if (be16(dpme->dpme_signature) != DPME_SIGNATURE)
    {
      screen_buffer_add("read_part_mac: bad DPME signature\n");
      return new_list_part;
    }

    int _insert_error = 0;
    partition_t new_partition(&arch_mac);
    new_partition.order = i;
    if (strcmp(dpme->dpme_type, "Apple_UNIX_SVR2") == 0)
    {
      if (!strcmp(dpme->dpme_name, "Swap") || !strcmp(dpme->dpme_name, "swap"))
        new_partition.part_type_mac = PMAC_SWAP;
      else
        new_partition.part_type_mac = PMAC_LINUX;
    }
    else if (strcmp(dpme->dpme_type, "Apple_Bootstrap") == 0)
      new_partition.part_type_mac = PMAC_NewWorld;
    else if (strcmp(dpme->dpme_type, "Apple_Scratch") == 0)
      new_partition.part_type_mac = PMAC_SWAP;
    else if (strcmp(dpme->dpme_type, "Apple_Driver") == 0)
      new_partition.part_type_mac = PMAC_DRIVER;
    else if (strcmp(dpme->dpme_type, "Apple_Driver43") == 0)
      new_partition.part_type_mac = PMAC_DRIVER43;
    else if (strcmp(dpme->dpme_type, "Apple_Driver_ATA") == 0)
      new_partition.part_type_mac = PMAC_DRIVERATA;
    else if (strcmp(dpme->dpme_type, "Apple_Driver_IOKit") == 0)
      new_partition.part_type_mac = PMAC_DRIVERIO;
    else if (strcmp(dpme->dpme_type, "Apple_Free") == 0)
      new_partition.part_type_mac = PMAC_FREE;
    else if (strcmp(dpme->dpme_type, "Apple_FWDriver") == 0)
      new_partition.part_type_mac = PMAC_FWDRIVER;
    else if (strcmp(dpme->dpme_type, "Apple_partition_map") == 0)
      new_partition.part_type_mac = PMAC_MAP;
    else if (strcmp(dpme->dpme_type, "Apple_Patches") == 0)
      new_partition.part_type_mac = PMAC_PATCHES;
    else if (strcmp(dpme->dpme_type, "Apple_HFS") == 0)
      new_partition.part_type_mac = PMAC_HFS;
    else if (strcmp(dpme->dpme_type, "Apple_MFS") == 0)
      new_partition.part_type_mac = PMAC_MFS;
    else if (strcmp(dpme->dpme_type, "Apple_PRODOS") == 0)
      new_partition.part_type_mac = PMAC_PRODOS;
    else if (strcmp(dpme->dpme_type, "Be_BFS") == 0)
      new_partition.part_type_mac = PMAC_BEOS;
    else if (strcmp(dpme->dpme_type, "DOS_FAT_32") == 0)
      new_partition.part_type_mac = PMAC_FAT32;
    else
    {
      new_partition.part_type_mac = PMAC_UNK;
      log_error("%s\n", dpme->dpme_type);
    }
    new_partition.part_offset =
        static_cast<uint64_t>(be32(dpme->dpme_pblock_start)) * PBLOCK_SIZE;
    new_partition.part_size =
        static_cast<uint64_t>(be32(dpme->dpme_pblocks)) * PBLOCK_SIZE;
    new_partition.status = STATUS_PRIM;
    check_part_mac(disk_car, verbose, new_partition, saveheader);
    aff_part_buffer(AFF_PART_ORDER | AFF_PART_STATUS, disk_car, new_partition);
    insert_new_partition(new_list_part, new_partition, 0, &_insert_error);
    if (i == 1)
    {
      limit = be32(dpme->dpme_map_entries);
    }
  }
  return new_list_part;
}

static auto write_part_mac(disk_t &disk_car, const list_part_t &list_part,
                           const int ro, const int verbose) -> int
{
  /* TODO: Implement it */
  if (ro == 0)
    return -1;
  return 0;
}

static void init_part_order_mac(const disk_t &disk_car, list_part_t &list_part)
{
  ;
}

void add_partition_mac_cli(disk_t &disk_car, list_part_t &list_part,
                           char **current_cmd)
{
  partition_t new_partition(&arch_mac);
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
                         4096 / disk_car.sector_size,
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
      //change_part_type_cli(disk_car, new_partition, current_cmd);
    }
    else if (new_partition.part_size > 0 && new_partition.part_type_mac > 0)
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
      if (test_structure_mac(list_part) != 0)
        new_partition.status = STATUS_DELETED;
      /*@ assert valid_list_part(list_part); */
      return;
    }
    /*@ assert valid_list_part(list_part); */
  }
}

static void set_next_status_mac(const disk_t &disk_car, partition_t &partition)
{
  if (partition.status == STATUS_DELETED)
    partition.status = STATUS_PRIM;
  else
    partition.status = STATUS_DELETED;
}

auto test_structure_mac(const list_part_t &list_part) -> int
{ /* Return 1 if bad*/
  list_part_t new_list_part;
  int res;
  new_list_part = gen_sorted_partition_list(list_part);
  res           = is_part_overlapping(new_list_part);
  return res;
}

static auto set_part_type_mac(partition_t &partition,
                              unsigned int part_type_mac) -> int
{
  if (part_type_mac > 0 && part_type_mac <= 255)
  {
    partition.part_type_mac = part_type_mac;
    return 0;
  }
  return 1;
}

static auto is_part_known_mac(const partition_t &partition) -> int
{
  return (partition.part_type_mac != PMAC_UNK);
}

static void init_structure_mac(const disk_t &disk_car, list_part_t &list_part,
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
  if (test_structure_mac(new_list_part))
  {
    for (partition_t &element : new_list_part)
      element.status = STATUS_DELETED;
  }
  list_part = new_list_part;
}

static auto check_part_mac(disk_t &disk_car, const int verbose,
                           partition_t &partition, const int saveheader) -> int
{
  int ret = 0;
  switch (partition.part_type_mac)
  {
  case PMAC_DRIVER43:
  case PMAC_DRIVERATA:
  case PMAC_DRIVERIO:
  case PMAC_FREE:
  case PMAC_FWDRIVER:
  case PMAC_SWAP:
  case PMAC_MAP:
  case PMAC_PATCHES:
  case PMAC_UNK:
  case PMAC_NewWorld:
  case PMAC_DRIVER:
  case PMAC_MFS:
  case PMAC_BEOS:
  case PMAC_PRODOS:
    break;
  case PMAC_LINUX:
    ret = check_linux(disk_car, partition, verbose);
    if (ret != 0)
      screen_buffer_add("No ext2, JFS, Reiser, cramfs or XFS marker\n");
    break;
  case PMAC_HFS:
    ret = check_HFSP(disk_car, partition, verbose);
    if (ret != 0)
    {
      ret = check_HFS(disk_car, partition, verbose);
    }
    break;
  case PMAC_FAT32:
    ret = check_FAT(disk_car, partition, verbose);
    break;
  default:
    if (verbose > 0)
    {
      log_info("check_part_mac {} type %02X: no test\n", partition.order,
               partition.part_type_mac);
    }
    break;
  }
  if (ret != 0)
  {
    log_error("check_part_mac failed for partition type %02X\n",
              partition.part_type_mac);
    aff_part_buffer(AFF_PART_ORDER | AFF_PART_STATUS, disk_car, partition);
    if (saveheader > 0)
    {
      save_header(disk_car, partition, verbose);
    }
  }
  return ret;
}

static auto get_partition_typename_mac_aux(const unsigned int part_type_mac)
    -> std::string_view
{
  for (const auto &mac_sys_type : mac_sys_types)
    if (mac_sys_type.part_type == part_type_mac)
      return mac_sys_type.name;
  return "";
}

static auto get_partition_typename_mac(const partition_t &partition) -> std::string_view
{
  return get_partition_typename_mac_aux(partition.part_type_mac);
}
#endif
