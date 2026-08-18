/*

    File: fnctdsk.h

    Copyright (C) 1998-2007 Christophe GRENIER <grenier@cgsecurity.org>

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
#ifndef _FNCTDSK_H
#define _FNCTDSK_H
#include "src/common.hpp"
#include <cstdint>

    /*@
      @ requires \valid_read(disk_car);
      @ requires valid_disk(disk_car);
      @ assigns \nothing;
      @*/
    auto C_H_S2LBA(const disk_t &disk_car, const unsigned int C, const unsigned int H,
                                const unsigned int S) -> unsigned long int;

    /*@
      @ requires \valid_read(disk_car);
      @ requires valid_disk(disk_car);
      @ requires \valid_read(CHS);
      @ assigns \nothing;
      @*/
    auto CHS2offset(const disk_t &disk_car, const CHS_t *CHS) -> uint64_t;

    /*@
      @ requires \valid_read(disk_car);
      @ requires valid_disk(disk_car);
      @ requires disk_car->sector_size > 0;
      @ requires disk_car->geom.sectors_per_head > 0;
      @ assigns \nothing;
      @ ensures 0 < \result <= disk_car->geom.sectors_per_head;
      @*/
    auto offset2sector(const disk_t &disk_car, const uint64_t offset) -> unsigned int;

    /*@
      @ requires \valid_read(disk_car);
      @ requires valid_disk(disk_car);
      @ requires disk_car->sector_size > 0;
      @ requires disk_car->geom.sectors_per_head > 0;
      @ requires disk_car->geom.heads_per_cylinder > 0;
      @ assigns \nothing;
      @ ensures \result <= disk_car->geom.heads_per_cylinder;
      @*/
    auto offset2head(const disk_t &disk_car, const uint64_t offset) -> unsigned int;

    /*@
      @ requires \valid_read(disk_car);
      @ requires valid_disk(disk_car);
      @ requires disk_car->sector_size > 0;
      @ requires disk_car->geom.sectors_per_head > 0;
      @ requires disk_car->geom.heads_per_cylinder > 0;
      @ assigns \nothing;
      @*/
    auto offset2cylinder(const disk_t &disk_car, const uint64_t offset) -> unsigned int;

    /*@
      @ requires \valid_read(disk_car);
      @ requires valid_disk(disk_car);
      @ requires \valid(CHS);
      @ requires disk_car->sector_size > 0;
      @ requires disk_car->geom.sectors_per_head > 0;
      @ requires disk_car->geom.heads_per_cylinder > 0;
      @ requires \separated(disk_car, CHS);
      @ assigns CHS->cylinder,CHS->head,CHS->sector;
      @*/
    void offset2CHS(const disk_t &disk_car, const uint64_t offset, CHS_t *CHS);

    /*@
      @ requires valid_list_disk(list_disk);
      @ requires \valid(disk);
      @ requires valid_disk(disk);
      @ requires disk==\null || \separated(disk, \union(list_disk, the_disk));
      @ requires the_disk==\null || (\valid(the_disk) && valid_disk(*the_disk) && \separated(the_disk, \union(list_disk,
      disk)));
      @ decreases 0;
      @*/
    // ensures \result==\null || (\valid(\result) && valid_disk(\result->disk));
    // ensures valid_list_disk(\result);
    // ensures disk==\null ==> \result == list_disk;
    // ensures the_disk==\null || (\valid_read(the_disk) && valid_disk(*the_disk));
    void insert_new_disk_aux(list_disk_t &list_disk, disk_t &disk, disk_t **the_disk);

    /*@
      @ requires list_disk==\null || valid_disk(list_disk->disk);
      @ requires \valid(disk_car);
      @ requires valid_disk(disk_car);
      @ requires valid_list_disk(list_disk);
      @ requires (list_disk==\null && disk_car==\null) || \separated(list_disk, disk_car);
      @*/
    // ensures \result==\null || \valid(\result);
    // ensures disk_car==\null ==> \result == list_disk;
    // ensures valid_list_disk(\result);
    void insert_new_disk(list_disk_t &list_disk, disk_t &disk_car);

    /*@
      @ requires list_part == \null || \valid(list_part);
      @ requires valid_list_part(list_part);
      @ requires valid_partition(part);
      @ requires \valid(insert_error);
      @ requires (list_part==\null && part==\null) || \separated(list_part, part, insert_error);
      @ ensures  valid_list_part(\result);
      @*/
    void insert_new_partition(list_part_t &list_part, partition_t &part, const int force_insert,
                                      int *insert_error);

    /*@
      @ requires \valid(list_part);
      @ requires valid_list_part(list_part);
      @ ensures  valid_list_part(\result);
      @*/
    void sort_partition_list(list_part_t &list_part);

    /*@
      @ requires \valid_read(list_part);
      @ requires valid_list_part(list_part);
      @ ensures  valid_list_part(\result);
      @*/
    auto gen_sorted_partition_list(const list_part_t &list_part) -> list_part_t;

    /*@
      @ requires \valid_read(disk_car);
      @ requires valid_disk(disk_car);
      @ requires \valid_read(list_part);
      @ requires valid_list_part(list_part);
      @*/
    // assigns \nothing;
    auto get_geometry_from_list_part(const disk_t &disk_car, const list_part_t &list_part, const int verbose) -> unsigned int;

    /*@
      @ requires valid_list_disk(list_disk);
      @ requires list_disk==\null || \freeable(list_disk);
      @ requires list_disk==\null || \freeable(list_disk->disk);
      @ requires list_disk==\null || (list_disk->disk->device == \null || \freeable(list_disk->disk->device));
      @ requires list_disk==\null || (list_disk->disk->model == \null || \freeable(list_disk->disk->model));
      @ requires list_disk==\null || (list_disk->disk->serial_no == \null || \freeable(list_disk->disk->serial_no));
      @ requires list_disk==\null || (list_disk->disk->fw_rev == \null || \freeable(list_disk->disk->fw_rev));
      @ requires list_disk==\null || (list_disk->disk->data == \null || \freeable(list_disk->disk->data));
      @ requires list_disk==\null || (list_disk->disk->rbuffer == \null || \freeable(list_disk->disk->rbuffer));
      @ requires list_disk==\null || (list_disk->disk->wbuffer == \null || \freeable(list_disk->disk->wbuffer));
      @ decreases 0;
      @*/
    auto delete_list_disk(list_disk_t &list_disk) -> int;

    /*@
      @ requires \valid(buffer + (0..99));
      @ ensures valid_string(buffer);
      @ assigns buffer[0 .. 99];
      @*/
    void size_to_unit(const uint64_t disk_size, char *buffer);

    /*@
      @ requires \valid_read(list_part);
      @ requires valid_list_part(list_part);
      @ assigns \nothing;
      @*/
    auto is_part_overlapping(const list_part_t &list_part) -> int;

    /*@
      @ requires valid_list_disk(list_disk);
      @ assigns \nothing;
      @*/
    void log_disk_list(list_disk_t &list_disk);
#endif
