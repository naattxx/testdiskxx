/*

    File: hdcache.c

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
#include <algorithm>
#include <iostream>
#include <string_view>
#include <utility>
#if !defined(DISABLED_FOR_FRAMAC)
#include <cstdio>
#include <cstdlib>
#include <cstring>
// #include "types.h"
#include "common.hpp"
#include "hdcache.hpp"
#include "log.hpp"

#define CACHE_BUFFER_NBR 16
#define CACHE_DEFAULT_SIZE 64 * 512
// #define DEBUG_CACHE 1

struct cache_buffer_struct
{
    unsigned char *buffer;
    unsigned int buffer_size;
    unsigned int cache_size;
    uint64_t cache_offset;
    int cache_status;
};

struct cache_struct
{
    disk_t *disk_car;
    struct cache_buffer_struct cache[CACHE_BUFFER_NBR];
#ifdef DEBUG_CACHE
    uint64_t nbr_fnct_sect;
    uint64_t nbr_pread_sect;
    unsigned int nbr_fnct_call;
    unsigned int nbr_pread_call;
#endif
    unsigned int cache_buffer_nbr;
    unsigned int cache_size_min;
    unsigned int last_io_error_nbr;
};

/*@
  @ requires \valid(disk_car);
  @ requires valid_disk(disk_car);
  @ requires \valid((char *)buffer + (0 .. count-1));
  @ requires separation: \separated(disk_car, (char *)buffer + (0 .. count-1));
  @*/
static auto cache_pread_aux(disk_t &disk_car, void *buffer, const unsigned int count, const uint64_t offset,
                           const unsigned int read_ahead) -> int
{
    auto *data = static_cast<struct cache_struct *>(disk_car.data);
#ifdef DEBUG_CACHE
    log_info("cache_pread(buffer, count={}, offset={}, read_ahead={})", count, (long long unsigned)offset,
             read_ahead);
    data->nbr_fnct_call++;
#endif
    {
        unsigned int i;
        unsigned int cache_buffer_nbr;
        /* Data is probably in the last buffers */
        for (i = 0, cache_buffer_nbr = data->cache_buffer_nbr; i < CACHE_BUFFER_NBR;
             i++, cache_buffer_nbr = (cache_buffer_nbr + CACHE_BUFFER_NBR - 1) % CACHE_BUFFER_NBR)
        {
            const struct cache_buffer_struct *cache = &data->cache[cache_buffer_nbr];
            if (cache->cache_offset <= offset && offset < cache->cache_offset + cache->cache_size &&
                cache->buffer != nullptr && cache->cache_size > 0)
            {
                const unsigned int data_available = cache->cache_size + cache->cache_offset - offset;
                const int res = cache->cache_status + cache->cache_offset - offset;
                if (count <= data_available)
                {
#ifdef DEBUG_CACHE
                    log_info("cache use {:5} count={}, coffset={}, cstatus={}", cache_buffer_nbr, cache->cache_size,
                             (long long unsigned)cache->cache_offset, cache->cache_status);
                    data->nbr_fnct_sect += count;
#endif
                    memcpy(buffer, cache->buffer + offset - cache->cache_offset, count);
                    return (std::cmp_less(res ,count) ? res : static_cast<signed>(count));
                }
                else
                {
#ifdef DEBUG_CACHE
                    log_info("cache USE {:5} count={}, coffset={}, ctstatus={}, call again cache_pread_aux",
                             cache_buffer_nbr, cache->cache_size, (long long unsigned)cache->cache_offset,
                             cache->cache_status);
                    data->nbr_fnct_sect += data_available;
#endif
                    memcpy(buffer, cache->buffer + offset - cache->cache_offset, data_available);
                    return res + cache_pread_aux(disk_car, static_cast<unsigned char *>(buffer) + data_available,
                                                 count - data_available, offset + data_available, read_ahead);
                }
            }
        }
    }
    {
        struct cache_buffer_struct *cache;
        const unsigned int count_new = (read_ahead != 0 && count < data->cache_size_min &&
                                                (offset + data->cache_size_min < data->disk_car->disk_real_size)
                                            ? data->cache_size_min
                                            : count);
        data->cache_buffer_nbr = (data->cache_buffer_nbr + 1) % CACHE_BUFFER_NBR;
        cache = &data->cache[data->cache_buffer_nbr];
        if (cache->buffer_size < count_new)
        { /* Buffer is too small, drop it */
            delete (cache->buffer);
            cache->buffer = nullptr;
        }
        if (cache->buffer == nullptr)
        { /* Allocate buffer */
            cache->buffer_size = (count_new < CACHE_DEFAULT_SIZE ? CACHE_DEFAULT_SIZE : count_new);
            cache->buffer = new unsigned char[cache->buffer_size];
        }
        cache->cache_size = count_new;
        cache->cache_offset = offset;
        cache->cache_status = data->disk_car->pread(*data->disk_car, cache->buffer, count_new, offset);
#ifdef DEBUG_CACHE
        data->nbr_fnct_sect += count;
        data->nbr_pread_call++;
        data->nbr_pread_sect += count_new;
        log_info("cache PREAD(buffer[{}], count={}, count_new={}, offset={}, cstatus={})", data->cache_buffer_nbr,
                 count, count_new, (long long unsigned)offset, cache->cache_status);
#endif
        if (std::cmp_greater_equal(cache->cache_status ,count))
        {
            data->last_io_error_nbr = 0;
            memcpy(buffer, cache->buffer, count);
            return count;
        }
        /* Read failure */
        data->last_io_error_nbr++;
        if (count_new <= disk_car.sector_size || disk_car.sector_size <= 0 || data->last_io_error_nbr > 1)
        {
            memcpy(buffer, cache->buffer, count);
            return cache->cache_status;
        }
        /* Free the existing cache */
        cache->cache_size = 0;
        /* split the read sector by sector */
        {
            unsigned int off;
            memset(buffer, 0, count);
            for (off = 0; off < count; off += disk_car.sector_size)
            {
                if (cache_pread_aux(disk_car, static_cast<unsigned char *>(buffer) + off,
                                    (disk_car.sector_size < count - off ? disk_car.sector_size : count - off),
                                    offset + off, 0) <= 0)
                {
                    return off;
                }
            }
            return count;
        }
    }
}

/*@
  @ requires \valid(disk_car);
  @ requires valid_disk(disk_car);
  @ requires \valid((char *)buffer + (0 .. count-1));
  @ requires separation: \separated(disk_car, (char *)buffer + (0 .. count-1));
  @*/
static auto cache_pread(disk_t &disk_car, void *buffer, const unsigned int count, const uint64_t offset) -> int
{
    const auto *data = static_cast<const struct cache_struct *>(disk_car.data);
    return cache_pread_aux(disk_car, buffer, count, offset, (data->last_io_error_nbr == 0));
}

/*@
  @ requires \valid(disk_car);
  @ requires valid_disk(disk_car);
  @ requires \valid_read((char *)buffer + (0 .. count-1));
  @ requires separation: \separated(disk_car, (const char *)buffer + (0 .. count-1));
  @ decreases 0;
  @*/
static auto cache_pwrite(disk_t &disk_car, const void *buffer, const unsigned int count, const uint64_t offset) -> int
{
    auto *data = static_cast<struct cache_struct *>(disk_car.data);
    unsigned int i;
    for (i = 0; i < CACHE_BUFFER_NBR; i++)
    {
        struct cache_buffer_struct *cache = &data->cache[i];
        if (!(cache->cache_offset + cache->cache_size - 1 < offset || offset + count - 1 < cache->cache_offset))
        {
            /* Discard the cache */
            cache->cache_size = 0;
        }
    }
    disk_car.write_used = 1;
    return data->disk_car->pwrite(*data->disk_car, buffer, count, offset);
}

/*@
  @ requires \valid(disk_car);
  @ requires valid_disk(disk_car);
  @ decreases 0;
  @*/
static void cache_clean(disk_t &disk_car)
{
    if (disk_car.data)
    {
        auto *data = static_cast<struct cache_struct *>(disk_car.data);
        unsigned int i;
#ifdef DEBUG_CACHE
        log_info("{}\ncache_pread total_call={}, total_count={}\n      read total_call={}, total_count={}\n",
                 data->disk_car.description(*data->disk_car), data->nbr_fnct_call,
                 (long long unsigned)data->nbr_fnct_sect, data->nbr_pread_call,
                 (long long unsigned)data->nbr_pread_sect);
#endif
        data->disk_car->clean(*data->disk_car);
        for (i = 0; i < CACHE_BUFFER_NBR; i++)
        {
            struct cache_buffer_struct *cache = &data->cache[i];
            delete (cache->buffer);
        }
        delete static_cast<struct cache_struct *>(disk_car.data);
        disk_car.data = nullptr;
    }
}

/*@
  @ requires \valid(disk_car);
  @ decreases 0;
  @*/
static auto cache_sync(disk_t &disk_car) -> int
{
    auto *data = static_cast<struct cache_struct *>(disk_car.data);
    return data->disk_car->sync(*data->disk_car);
}

/*@
  @ requires \valid_read(CHS_source);
  @ requires \valid(CHS_dst);
  @ requires separation: \separated(CHS_dst, CHS_source);
  @ assigns CHS_dst->cylinders, CHS_dst->heads_per_cylinder, CHS_dst->sectors_per_head;
  @ ensures CHS_dst->cylinders==CHS_source->cylinders;
  @ ensures CHS_dst->heads_per_cylinder==CHS_source->heads_per_cylinder;
  @ ensures CHS_dst->sectors_per_head==CHS_source->sectors_per_head;
  @*/
static void dup_geometry(CHSgeometry_t *CHS_dst, const CHSgeometry_t *CHS_source)
{
    CHS_dst->cylinders = CHS_source->cylinders;
    CHS_dst->heads_per_cylinder = CHS_source->heads_per_cylinder;
    CHS_dst->sectors_per_head = CHS_source->sectors_per_head;
}

/*@
  @ requires \valid(disk_car);
  @ requires valid_disk(disk_car);
  @ decreases 0;
  @ ensures valid_read_string(\result);
  @*/
static auto cache_description(disk_t &disk_car) -> std::string_view
{
    std::string_view tmp;
    auto *data = static_cast<struct cache_struct *>(disk_car.data);
    dup_geometry(&data->disk_car->geom, &disk_car.geom);
    data->disk_car->disk_size = disk_car.disk_size;
    tmp = data->disk_car->description(*data->disk_car);
    /*@ assert valid_read_string(tmp); */
    return tmp;
}

/*@
  @ requires \valid(disk_car);
  @ requires valid_disk(disk_car);
  @ decreases 0;
  @ ensures valid_read_string(\result);
  @*/
static auto cache_description_short(disk_t &disk_car) -> std::string_view
{
    std::string_view tmp;
    auto *data = static_cast<struct cache_struct *>(disk_car.data);
    dup_geometry(&data->disk_car->geom, &disk_car.geom);
    data->disk_car->disk_size = disk_car.disk_size;
    tmp = data->disk_car->description_short(*data->disk_car);
    /*@ assert valid_read_string(tmp); */
    return tmp;
}

auto new_diskcache(disk_t &disk_car, const unsigned int testdisk_mode) -> disk_t
{
    unsigned int i;
    auto *data = new struct cache_struct;
    disk_t new_disk_car = disk_car;
    data->disk_car = &disk_car;
#ifdef DEBUG_CACHE
    data->nbr_fnct_sect = 0;
    data->nbr_pread_sect = 0;
    data->nbr_fnct_call = 0;
    data->nbr_pread_call = 0;
#endif
    data->cache_buffer_nbr = 0;
    data->last_io_error_nbr = 0;
    if (testdisk_mode & TESTDISK_O_READAHEAD_8K)
        data->cache_size_min = 16 * 512;
    else if (testdisk_mode & TESTDISK_O_READAHEAD_32K)
        data->cache_size_min = 64 * 512;
    else
        data->cache_size_min = 0;
    dup_geometry(&new_disk_car.geom, &disk_car.geom);
    new_disk_car.disk_size = disk_car.disk_size;
    new_disk_car.disk_real_size = disk_car.disk_real_size;
    new_disk_car.write_used = 0;
    new_disk_car.data = data;
    new_disk_car.pread = &cache_pread;
    new_disk_car.pwrite = &cache_pwrite;
    new_disk_car.sync = &cache_sync;
    new_disk_car.clean = &cache_clean;
    new_disk_car.description = &cache_description;
    new_disk_car.description_short = &cache_description_short;
    new_disk_car.rbuffer = nullptr;
    new_disk_car.wbuffer = nullptr;
    new_disk_car.rbuffer_size = 0;
    new_disk_car.wbuffer_size = 0;
    for (i = 0; i < CACHE_BUFFER_NBR; i++)
    {
        data->cache[i].buffer = nullptr;
        data->cache[i].buffer_size = 0;
    }
    std::swap(new_disk_car,disk_car);
    return new_disk_car;
}
#endif
