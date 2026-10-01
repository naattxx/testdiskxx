/*

    File: alignio.h

    Copyright (C) 2008 Christophe GRENIER <grenier@cgsecurity.org>

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

#ifndef _ALIGNIO_H
#define _ALIGNIO_H
#include <utility>

#include "common.hpp"
#include "log.hpp"

/*@
  @ requires \valid_function(fnct_pread);
  @ requires \valid(disk_car);
  @ requires valid_disk(disk_car);
  @ requires disk_car->sector_size > 0;
  @ requires disk_car->offset < 0x8000000000000000;
  @ requires 0 < count < 0x8000000000000000;
  @ requires offset < 0x8000000000000000;
  @ requires \valid((char *)buf + (0 .. count -1));
  @ requires \separated(disk_car, (char *)buf);
  @ decreases 0;
  @ ensures  valid_disk(disk_car);
  @*/
static auto align_pread(int (*fnct_pread)(const disk_t &disk_car, void *buf, const unsigned int count,
                                         const uint64_t offset),
                       disk_t &disk_car, void *buf, const unsigned int count, const uint64_t offset) -> int
{
    const uint64_t offset_new = offset + disk_car.offset;
    const unsigned int count_new = ((offset_new % disk_car.sector_size) + count + disk_car.sector_size - 1) /
                                   disk_car.sector_size * disk_car.sector_size;
    /*@ assert count_new >= count; */
    /*@ assert count_new >= disk_car->sector_size; */
    /*@ assert count_new > 0; */
    if (count != count_new ||
        ((disk_car.access_mode & TESTDISK_O_DIRECT) != 0 &&
         (((size_t)buf & (disk_car.sector_size - 1)) != 0) &&
         (buf != disk_car.rbuffer.data() || disk_car.rbuffer.size() < count_new)))
    {
        int res;
        if (disk_car.rbuffer.size() < count_new)
        {
            disk_car.rbuffer.clear();
        }
        if (disk_car.rbuffer.empty())
        {
            unsigned int size = 128 * 512;
            while (size < count_new)
            {
                size *= 2;
            }
            /*@ assert size >= count_new; */
            disk_car.rbuffer.resize(size);
            /*@ assert valid_disk(disk_car); */
        }
        /*@ assert \freeable(disk_car.rbuffer); */
        /*@ assert valid_disk(disk_car); */
        res = fnct_pread(disk_car, disk_car.rbuffer.data(), count_new,
                         offset_new / disk_car.sector_size * disk_car.sector_size);
        memcpy(buf, disk_car.rbuffer.data() + (offset_new % disk_car.sector_size), count);
        /*@ assert \freeable(disk_car.rbuffer.data()) && disk_car.rbuffer.size() > 0; */
        /*@ assert valid_disk(disk_car); */
        return (std::cmp_less(res ,count) ? res : static_cast<signed>(count));
    }
    /*@ assert valid_disk(disk_car); */
    return fnct_pread(disk_car, buf, count, offset_new);
}

/*@
  @ requires \valid_function(fnct_pread);
  @ requires \valid_function(fnct_pwrite);
  @ requires \valid(disk_car);
  @ requires valid_disk(disk_car);
  @ requires disk_car->sector_size > 0;
  @ requires disk_car->offset < 0x8000000000000000;
  @ requires 0 < count < 0x8000000000000000;
  @ requires offset < 0x8000000000000000;
  @ requires \valid_read((char *)buf + (0 .. count -1));
  @ requires \separated(disk_car, (char *)buf);
  @ decreases 0;
  @ ensures  valid_disk(disk_car);
  @*/
static auto align_pwrite(int (*fnct_pread)(const disk_t &disk_car, void *buf, const unsigned int count,
                                          const uint64_t offset),
                        int (*fnct_pwrite)(disk_t &disk_car, const void *buf, const unsigned int count,
                                           const uint64_t offset),
                        disk_t &disk_car, const void *buf, const unsigned int count, const uint64_t offset) -> int
{
    const uint64_t offset_new = offset + disk_car.offset;
    const unsigned int count_new = ((offset_new % disk_car.sector_size) + count + disk_car.sector_size - 1) /
                                   disk_car.sector_size * disk_car.sector_size;
    if (count != count_new ||
        ((disk_car.access_mode & TESTDISK_O_DIRECT) != 0 &&
         (((size_t)buf & (disk_car.sector_size - 1)) != 0)))
    {
        int tmp;
        if (disk_car.wbuffer.size() < count_new)
        {
            disk_car.wbuffer.clear();
        }
        if (disk_car.wbuffer.empty())
        {
            unsigned int size = 128 * 512;
            /*@ loop assigns size; */
            while (size < count_new)
            {
                size *= 2;
            }
            /*@ assert size >= count_new; */
            disk_car.wbuffer.resize(size);
            /*@ assert valid_disk(disk_car); */
        }
        /*@ assert \freeable(disk_car->wbuffer); */
        /*@ assert valid_disk(disk_car); */
        if (fnct_pread(disk_car, disk_car.wbuffer.data(), count_new,
                       offset_new / disk_car.sector_size * disk_car.sector_size) < 0)
        {
            log_error("read failed but trying to write anyway");
            memset(disk_car.wbuffer.data(), 0, disk_car.wbuffer.size());
        }
        memcpy(disk_car.wbuffer.data() + (offset_new % disk_car.sector_size), buf, count);
        tmp = fnct_pwrite(disk_car, disk_car.wbuffer.data(), count_new,
                          offset_new / disk_car.sector_size * disk_car.sector_size);
        /*@ assert \freeable(disk_car.wbuffer) && disk_car.wbuffer_size > 0; */
        /*@ assert valid_disk(disk_car); */
        return (std::cmp_less(tmp ,count) ? tmp : static_cast<signed>(count));
    }
    /*@ assert valid_disk(disk_car); */
    return fnct_pwrite(disk_car, buf, count, offset_new);
}
#endif
