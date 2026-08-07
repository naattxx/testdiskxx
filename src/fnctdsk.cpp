/*

    File: fnctdsk.c

    Copyright (C) 1998-2005,2008 Christophe GRENIER <grenier@cgsecurity.org>

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

#include <config.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include "types.h"
#include "common.hpp"
#include "fnctdsk.hpp"
#include "log.hpp"
#include "log_part.hpp"
// #include "guid_cpy.hpp"

unsigned long int C_H_S2LBA(const disk_t &disk_car, const unsigned int C, const unsigned int H, const unsigned int S)
{
    return ((unsigned long int)C * disk_car.geom.heads_per_cylinder + H) * disk_car.geom.sectors_per_head + S - 1;
}

uint64_t CHS2offset(const disk_t &disk_car, const CHS_t *CHS)
{
    return (((uint64_t)CHS->cylinder * disk_car.geom.heads_per_cylinder + CHS->head) *
                disk_car.geom.sectors_per_head +
            CHS->sector - 1) *
           disk_car.sector_size;
    //  return (uint64_t)C_H_S2LBA(disk_car, CHS->cylinder, CHS->head, CHS->sector) * disk_car.sector_size;
}

unsigned int offset2sector(const disk_t &disk_car, const uint64_t offset)
{
    return ((offset / disk_car.sector_size) % disk_car.geom.sectors_per_head) + 1;
}

unsigned int offset2head(const disk_t &disk_car, const uint64_t offset)
{
    return ((offset / disk_car.sector_size) / disk_car.geom.sectors_per_head) % disk_car.geom.heads_per_cylinder;
}

unsigned int offset2cylinder(const disk_t &disk_car, const uint64_t offset)
{
    return ((offset / disk_car.sector_size) / disk_car.geom.sectors_per_head) / disk_car.geom.heads_per_cylinder;
}

void offset2CHS(const disk_t &disk_car, const uint64_t offset, CHS_t *CHS)
{
    uint64_t pos = offset / disk_car.sector_size;
    CHS->sector = (pos % disk_car.geom.sectors_per_head) + 1;
    pos /= disk_car.geom.sectors_per_head;
    CHS->head = pos % disk_car.geom.heads_per_cylinder;
    CHS->cylinder = pos / disk_car.geom.heads_per_cylinder;
}

/*@
  @ requires valid_list_disk(list_disk);
  @ requires disk!=\null;
  @ requires valid_disk(disk);
  @ assigns \nothing;
  @*/
static disk_t *search_disk(const list_disk_t &list_disk, const disk_t &disk)
{
    /*@
      @ loop assigns tmp;
      @*/
    for (const disk_t& tmp : list_disk)
    {
        if (!tmp.device.empty() && !disk.device.empty() && tmp.device == disk.device)
        {
            return const_cast<disk_t*>(&tmp);
        }
    }
    return nullptr;
}

void insert_new_disk_aux(list_disk_t &list_disk, disk_t &disk, disk_t **the_disk)
{
    //list_disk_t result(list_disk);
    disk_t* found;
    // if (!disk)
    // {
    //     if (the_disk != nullptr)
    //     {
    //         /*@ assert \valid(the_disk); */
    //         *the_disk = nullptr;
    //     }
    //     /*@ assert valid_list_disk(list_disk); */
    //     return;
    // }
    found = search_disk(list_disk, disk);
    /* Do not add a disk already known */
    if (found != nullptr)
    {
        disk.clean(disk);
        if (the_disk != nullptr)
        {
            /*@ assert \valid(the_disk); */
            *the_disk = found;
        }
        /*@ assert valid_list_disk(list_disk); */
        return;
    }
    /* Add the disk at the end */
    list_disk.push_back(disk);
    if (the_disk != nullptr)
    {
        /*@ assert \valid(the_disk); */
        *the_disk = &disk;
    }
    /*@ assert valid_list_disk(new_disk); */
    /*@ assert valid_list_disk(list_disk); */

}

void insert_new_disk(list_disk_t &list_disk, disk_t &disk)
{
    insert_new_disk_aux(list_disk, disk, nullptr);
}

void insert_new_partition(list_part_t &list_part, partition_t &new_partition, const int force_insert, int *insert_error)
{
    *insert_error = 0;
    if (list_part.empty())
    {
        list_part.push_back(new_partition);
        return;
    }
    /*@
      @ loop invariant valid_list_part(list_part);
      @ loop invariant valid_partition(part);
      @ loop invariant \valid(insert_error);
      @*/
    for (partition_t partition : list_part)
    { /* prev new next */
        /*@ assert partition != \null || valid_partition(partition); */
        if ((new_partition.part_offset < partition.part_offset) ||
            (new_partition.part_offset == partition.part_offset &&
             ((new_partition.part_size < partition.part_size) ||
              (new_partition.part_size == partition.part_size &&
               (force_insert == 0 || new_partition.sb_offset < partition.sb_offset)))))
        {
            if (force_insert == 0 && (partition.part_offset == new_partition.part_offset) &&
                (partition.part_size == new_partition.part_size) && (partition.part_type_i386 == new_partition.part_type_i386) &&
                (partition.part_type_mac == new_partition.part_type_mac) &&
                (partition.part_type_sun == new_partition.part_type_sun) &&
                (partition.part_type_xbox == new_partition.part_type_xbox) &&
                (partition.upart_type == new_partition.upart_type || new_partition.upart_type == UP_UNK))
            { /*CGR 2004/05/31*/
                if (partition.status == STATUS_DELETED)
                {
                    partition.status = new_partition.status;
                }
                *insert_error = 1;
                /*@ assert valid_list_part(list_part); */
                return;
            }
            { /* prev new_element next */
                list_part.push_back(new_partition);
                return;
            }
        }
    }
}

int delete_list_disk(list_disk_t& list_disk)
{
    int write_used = 0;
    /*@
      @ loop invariant valid_list_disk(element_disk);
      @*/
    for (disk_t &disk : list_disk)
    {
        /*@ assert valid_disk(disk); */
        write_used |= disk.write_used;
        /*@ assert \valid_read(disk); */
        /*@ assert \valid_function(disk.clean); */
        disk.clean(disk);
    }
    return write_used;
}

void sort_partition_list(list_part_t &list_part)
{
    list_part_t new_list_part;
    /*@ assert valid_list_part(new_list_part); */
    /*@
      @ loop invariant valid_list_part(list_part);
      @ loop invariant valid_list_part(new_list_part);
      @*/
    for (partition_t &element : list_part)
    {
        int insert_error = 0;
        /*@ assert \valid(element); */
        insert_new_partition(new_list_part, element, 0, &insert_error);
    }
    /*@ assert valid_list_part(new_list_part); */
    list_part = new_list_part;
}

list_part_t gen_sorted_partition_list(const list_part_t &list_part)
{
    list_part_t new_list_part;
    /*@ assert valid_list_part(new_list_part); */
    /*@
      @ loop invariant valid_list_part(list_part);
      @ loop invariant valid_list_part(new_list_part);
      @*/
    for (partition_t element : list_part)
    {
        /*@ assert \valid_read(element); */
        int _insert_error = 0;

        if (element.status != STATUS_DELETED)
            insert_new_partition(new_list_part, element, 1, &_insert_error);
        /*@ assert \valid_read(element); */
    }
    /*@ assert valid_list_part(new_list_part); */
    return new_list_part;
}

int is_part_overlapping(const list_part_t &list_part)
{

    /* Test overlapping
       Must be space between a primary/logical partition and a logical partition for an extended
    */
    if (list_part.empty())
        return 0;
    auto element = list_part.cbegin();
    /*@
      @ loop invariant \valid_read(element);
      @ loop assigns element;
      @*/
    while (1)
    {
        auto next = std::next(element);
        const partition_t &partition = *element;
        const partition_t &next_part = *next;
        if (next == list_part.cend())
            return 0;
        /*@ assert \valid_read(partition); */
        /*@ assert \valid_read(next_part); */
        if ((partition.part_offset + partition.part_size - 1 >= next_part.part_offset) ||
            ((partition.status == STATUS_PRIM || partition.status == STATUS_PRIM_BOOT ||
              partition.status == STATUS_LOG) &&
             next_part.status == STATUS_LOG &&
             partition.part_offset + partition.part_size - 1 + 1 >= next_part.part_offset))
            return 1;
        element = next;
    }
}

/*@
  @ requires \valid(partition);
  @ requires valid_partition(partition);
  @ requires \valid_read(arch);
  @ requires \separated(partition, arch);
  @ ensures partition.part_size == 0;
  @ ensures partition.sborg_offset == 0;
  @ ensures partition.sb_offset == 0;
  @ ensures partition.sb_size == 0;
  @ ensures partition.blocksize == 0;
  @ ensures partition.part_type_i386 == P_NO_OS;
  @ ensures partition.part_type_sun == PSUN_UNK;
  @ ensures partition.part_type_mac == PMAC_UNK;
  @ ensures partition.part_type_xbox == PXBOX_UNK;
  @ ensures partition.upart_type == UP_UNK;
  @ ensures partition.status == STATUS_DELETED;
  @ ensures partition.order == NO_ORDER;
  @ ensures partition.errcode == BAD_NOERR;
  @ ensures partition.fsname[0] == '\0';
  @ ensures partition.partname[0] == '\0';
  @ ensures partition.info[0] == '\0';
  @ ensures partition.arch == arch;
  @*/
// assigns partition.part_size;
// assigns partition.sborg_offset;
// assigns partition.sb_offset;
// assigns partition.sb_size;
// assigns partition.blocksize;
// assigns partition.part_type_i386;
// assigns partition.part_type_sun;
// assigns partition.part_type_mac;
// assigns partition.part_type_xbox;
// assigns partition.part_type_gpt;
// assigns partition.part_uuid;
// assigns partition.upart_type;
// assigns partition.status;
// assigns partition.order;
// assigns partition.errcode;
// assigns partition.fsname[0];
// assigns partition.partname[0];
// assigns partition.info[0];
void partition_t::reset(const arch_fnct_t *arch)
{
    /* lba=0; Don't reset lba, used by search_part */
    part_size = (uint64_t)0;
    sborg_offset = 0;
    sb_offset = 0;
    sb_size = 0;
    blocksize = 0;
    part_type_i386 = P_NO_OS;
    part_type_sun = PSUN_UNK;
    part_type_mac = PMAC_UNK;
    part_type_xbox = PXBOX_UNK;
    part_type_gpt = (const efi_guid_t)GPT_ENT_TYPE_UNUSED;
#ifndef DISABLED_FOR_FRAMAC
    part_uuid = GPT_ENT_TYPE_UNUSED;
    // guid_cpy(&part_uuid, &GPT_ENT_TYPE_UNUSED);
#endif
    upart_type = UP_UNK;
    status = STATUS_DELETED;
    order = NO_ORDER;
    errcode = BAD_NOERR;
    fsname[0] = '\0';
    partname[0] = '\0';
    info[0] = '\0';
    this->arch = arch;
}

/*@
  @ requires \valid_read(arch);
  @*/
// ensures valid_partition(\result);
// ensures \result->arch == arch;
partition_t::partition_t(const arch_fnct_t *arch)
{
    reset(arch);
}

/*@
  @ requires \valid_read(disk_car);
  @ requires \valid_read(list_part);
  @ assigns \nothing;
  @*/
static unsigned int get_geometry_from_list_part_aux(const disk_t &disk_car, const list_part_t &list_part,
                                                    const int verbose)
{
    unsigned int nbr = 0;
    /*@
      @ loop assigns element, nbr;
      @ loop invariant valid_list_part(element);
      @*/
    for (const partition_t &element : list_part)
    {
        CHS_t start;
        CHS_t end;
        /*@ assert \valid_read(element); */
        offset2CHS(disk_car, element.part_offset, &start);
        offset2CHS(disk_car, element.part_offset + element.part_size - 1, &end);
        if (start.sector == 1 && start.head <= 1)
        {
            nbr++;
            if (end.head == disk_car.geom.heads_per_cylinder - 1)
            {
                nbr++;
                /* Doesn't check if end.sector==disk_car.CHS.sector */
            }
        }
    }
#ifndef DISABLED_FOR_FRAMAC
    if (nbr > 0)
    {
        log_info("get_geometry_from_list_part_aux head={} nbr={}", disk_car.geom.heads_per_cylinder, nbr);
        if (verbose > 1)
        {
            for (const partition_t &element : list_part)
            {
                CHS_t start;
                CHS_t end;
                offset2CHS(disk_car, element.part_offset, &start);
                offset2CHS(disk_car, element.part_offset + element.part_size - 1, &end);
                if (start.sector == 1 && start.head <= 1 && end.head == disk_car.geom.heads_per_cylinder - 1)
                {
                    log_partition(disk_car, element);
                }
            }
        }
    }
#endif
    return nbr;
}

unsigned int get_geometry_from_list_part(const disk_t &disk_car, const list_part_t &list_part, const int verbose)
{
    const unsigned int head_list[] = {8, 16, 32, 64, 128, 240, 255, 0};
    unsigned int best_score;
    unsigned int i;
    unsigned int heads_per_cylinder = disk_car.geom.heads_per_cylinder;
    disk_t new_disk_car = disk_car;
    best_score = get_geometry_from_list_part_aux(new_disk_car, list_part, verbose);
    /*@ loop assigns i, best_score, heads_per_cylinder, new_disk_car.geom.heads_per_cylinder; */
    for (i = 0; head_list[i] != 0; i++)
    {
        unsigned int score;
        new_disk_car.geom.heads_per_cylinder = head_list[i];
        score = get_geometry_from_list_part_aux(new_disk_car, list_part, verbose);
        if (score >= best_score)
        {
            best_score = score;
            heads_per_cylinder = new_disk_car.geom.heads_per_cylinder;
        }
    }
    return heads_per_cylinder;
}

void size_to_unit(const uint64_t disk_size, char *buffer)
{
#ifdef DISABLED_FOR_FRAMAC
    buffer[0] = '\0';
#else
    if (disk_size < (uint64_t)10 * 1024)
        sprintf(buffer, "%u B", (unsigned)disk_size);
    else if (disk_size < (uint64_t)10 * 1024 * 1024)
        sprintf(buffer, "%u KB / %u KiB", (unsigned)(disk_size / 1000), (unsigned)(disk_size / 1024));
    else if (disk_size < (uint64_t)10 * 1024 * 1024 * 1024)
        sprintf(buffer, "%u MB / %u MiB", (unsigned)(disk_size / 1000 / 1000), (unsigned)(disk_size / 1024 / 1024));
    else if (disk_size < (uint64_t)10 * 1024 * 1024 * 1024 * 1024)
        sprintf(buffer, "%u GB / %u GiB", (unsigned)(disk_size / 1000 / 1000 / 1000),
                (unsigned)(disk_size / 1024 / 1024 / 1024));
    else
        sprintf(buffer, "%u TB / %u TiB", (unsigned)(disk_size / 1000 / 1000 / 1000 / 1000),
                (unsigned)(disk_size / 1024 / 1024 / 1024 / 1024));
#endif
}

void log_disk_list(list_disk_t &list_disk)
{
#ifndef DISABLED_FOR_FRAMAC
    /* save disk parameters to rapport */
    log_info("Hard disk list");
    /*@
      @ loop invariant valid_list_disk(list_disk);
      @ loop invariant valid_list_disk(element_disk);
      @*/
    for (disk_t& disk : list_disk)
    {
        std::string disk_description(disk.description(disk));
        disk_description.append(", sector size=").append(std::to_string(disk.sector_size));
        if (!disk.model.empty())
            disk_description.append(" - ").append(disk.model);
        if (!disk.serial_no.empty())
            disk_description.append(", S/N:").append(disk.serial_no);
        if (!disk.fw_rev.empty())
            disk_description.append(", FW:").append(disk.fw_rev);
        log_info("{}", disk_description);
    }
#endif
}
