/*

    File: intrf.c

    Copyright (C) 1998-2009 Christophe GRENIER <grenier@cgsecurity.org>

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

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <format>
#include <string>
#include <vector>

#if __has_include(<libgen.h>)
#include <libgen.h>
#endif
#if __has_include(<sys/select.h>)
#include <sys/select.h>
#endif
#if __has_include(<sys/stat.h>)
#include <sys/stat.h>
#endif
#if __has_include(<sys/time.h>)
#include <sys/time.h>
#endif
#if __has_include(<unistd.h>)
#include <unistd.h>
#endif
#if __has_include(<sys/cygwin.h>)
#include <sys/cygwin.h>
#endif
#include <cerrno>
// #include "types.h"
#include "common.hpp"
// #include "lang.h"
#include "fnctdsk.hpp"
#include "intrf.hpp"
// #include "dir.h"
#include "log.hpp"

char intr_buffer_screen[MAX_LINES][BUFFER_LINE_LENGTH + 1];
int intr_nbr_line = 0;

auto screen_buffer_add(const char *_format, ...) -> int
{
#ifndef DISABLED_FOR_FRAMAC
    char tmp[BUFFER_LINE_LENGTH + 1];
    const char *start = tmp;
    va_list ap;
    memset(tmp, '\0', sizeof(tmp));
    va_start(ap, _format);
    vsnprintf(tmp, sizeof(tmp), _format, ap);
    va_end(ap);
    while (start != nullptr && intr_nbr_line < MAX_LINES)
    {
        const unsigned int dst_current_len = strlen(intr_buffer_screen[intr_nbr_line]);
        const char *end = strchr(start, '\n');
        unsigned int nbr = (end == nullptr ? strlen(start) : static_cast<unsigned int>(end - start));
        nbr = std::min(nbr, BUFFER_LINE_LENGTH - dst_current_len);

        memcpy(&intr_buffer_screen[intr_nbr_line][dst_current_len], start, nbr);
        intr_buffer_screen[intr_nbr_line][dst_current_len + nbr] = '\0';
        if (end != nullptr)
        {
            if (++intr_nbr_line < MAX_LINES)
                intr_buffer_screen[intr_nbr_line][0] = '\0';
            end++;
        }
        start = end;
    }
    /*	log_trace("aff_intr_buffer_screen {} =>{}<=\n",intr_nbr_line,tmp); */
    if (intr_nbr_line == MAX_LINES)
    {
        log_warning("Buffer can't store more than {} lines.\n", MAX_LINES);
        intr_nbr_line++;
    }
#endif
    return 0;
}

/*@
  @ ensures intr_nbr_line == 0;
  @ assigns intr_nbr_line;
  @ assigns intr_buffer_screen[0 .. MAX_LINES-1][ 0 .. BUFFER_LINE_LENGTH];
  @*/
void screen_buffer_reset()
{
    intr_nbr_line = 0;
    memset(intr_buffer_screen, 0, sizeof(intr_buffer_screen));
}

void screen_buffer_to_log()
{
    int i;
    if (intr_buffer_screen[intr_nbr_line][0] != '\0')
        intr_nbr_line++;
    /* to log file */
    /*@
      @ loop variant intr_nbr_line - i;
      @*/
    for (i = 0; i < intr_nbr_line; i++)
        log_info("{}", intr_buffer_screen[i]);
}

auto aff_part_aux(const unsigned int newline, const disk_t &disk, const partition_t &partition) -> std::vector<std::string>
{
  assert(partition.arch != nullptr && "BUG: No arch for a partition");
  using std::string, std::to_string;

  std::vector<std::string> result;
  char status;

    if ((newline & AFF_PART_ORDER) == AFF_PART_ORDER)
    {
        if (partition.status != STATUS_EXT_IN_EXT && partition.order != NO_ORDER)
            result.push_back(to_string(partition.order));
    }
    if ((newline & AFF_PART_STATUS) == AFF_PART_STATUS)
    {
        status = partition.status;
        /* Don't marked as D(eleted) an entry that is not a partition */
        if ((newline & AFF_PART_ORDER) == AFF_PART_ORDER && partition.order == NO_ORDER &&
            partition.status == STATUS_DELETED)
            status = ' ';
        result.emplace_back(1, status);
    }
    if (partition.arch->get_partition_typename(partition) != nullptr)
        result.emplace_back(partition.arch->get_partition_typename(partition));
    else if (partition.arch->get_part_type)
        result.push_back(std::format("Sys={:02X}",
                      partition.arch->get_part_type(partition)));
    else
        result.emplace_back("Unknown");
    if (disk.unit == UNIT::SECTOR)
    {
      result.push_back(to_string(partition.part_offset / disk.sector_size));
      result.push_back(to_string((partition.part_offset + partition.part_size - 1) /
                        disk.sector_size));
    }
    else
    {
      result.push_back(std::format("{:5} {:3} {:2}",
                          offset2cylinder(disk, partition.part_offset),
                          offset2head(disk, partition.part_offset),
                          offset2sector(disk, partition.part_offset)));
      result.push_back(std::format(
          "{:5} {:3} {:2}",
          offset2cylinder(disk,
                          partition.part_offset + partition.part_size - 1),
          offset2head(disk, partition.part_offset + partition.part_size - 1),
          offset2sector(disk, partition.part_offset + partition.part_size - 1)
      ));
    }
    result.push_back(to_string(partition.part_size / disk.sector_size));
    if (partition.partname[0] != '\0')
        result.push_back("[" + partition.partname + "]");
    if (partition.fsname[0] != '\0')
        result.push_back("[" + partition.fsname + "]");

    return result;
}

#define PATH_SEP '/'
#ifdef __CYGWIN__
/* /cygdrive/c/ => */
#define PATH_DRIVE_LENGTH 9
#endif

auto atouint64(const char *nptr) -> uint64_t
{
    uint64_t tmp = 0;
    /*@
      @ loop invariant valid_read_string(nptr);
      @ loop assigns tmp, nptr;
      @*/
    while (*nptr >= '0' && *nptr <= '9')
    {
        tmp = tmp * 10 + *nptr - '0';
        nptr++;
    }
    return tmp;
}

auto ask_number_cli(char **current_cmd, const uint64_t val_cur, const uint64_t val_min, const uint64_t val_max,
                        const char *_format, ...) -> uint64_t
{
    /*@ assert \valid(current_cmd); */
    if (*current_cmd != nullptr)
    {
        uint64_t tmp_val;
        skip_comma_in_command(current_cmd);
        /*@ assert valid_read_string(*current_cmd); */
        tmp_val = get_int_from_command(current_cmd);
        /*@ assert valid_read_string(*current_cmd); */
        if (val_min == val_max || (tmp_val >= val_min && tmp_val <= val_max))
            return tmp_val;
#ifndef DISABLED_FOR_FRAMAC

        char res[200];
        va_list ap;
        va_start(ap, _format);
        vsnprintf(res, sizeof(res), _format, ap);
        log_error("{}", res);
        if (val_min != val_max)
          log_error("({}-{}) :", (long long unsigned)val_min,
                    (long long unsigned)val_max);
        log_error("Invalid value\n");
        va_end(ap);

#endif
    }
    /*@ assert valid_read_string(*current_cmd); */
    return val_cur;
}

void aff_part_buffer(const unsigned int newline, const disk_t &disk_car, const partition_t &partition)
{
    const auto msg = aff_part_aux(newline, disk_car, partition);
    screen_buffer_add("%s\n", std::format("{}", msg).c_str());
}

void log_CHS_from_LBA(const disk_t &disk_car, const unsigned long int pos_LBA)
{
    unsigned long int tmp;
    unsigned long int cylinder, head, sector;
    tmp = disk_car.geom.sectors_per_head;
    sector = (pos_LBA % tmp) + 1;
    tmp = pos_LBA / tmp;
    cylinder = tmp / disk_car.geom.heads_per_cylinder;
    head = tmp % disk_car.geom.heads_per_cylinder;
#ifndef DISABLED_FOR_FRAMAC
    log_info("{}/{}/{}", cylinder, head, sector);
#endif
}
