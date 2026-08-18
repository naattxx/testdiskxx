/*

    File: io_redir.c

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
#include <cerrno>
#include <utility>
#include <cstdio>
#include <cstdlib>
#include <cstring>
// #include "types.h"
#include "common.hpp"
#include "io_redir.hpp"
#include "log.hpp"

// #define DEBUG_IO_REDIR 1

struct list_redir_t
{
    uint64_t org_offset;
    uint64_t new_offset;
    unsigned int size;
    const void *mem;
    list_redir_t *prev;
    list_redir_t *next;
};

struct info_io_redir
{
    disk_t *disk_car;
    list_redir_t *list_redir;
};

/*@
  @ requires \valid(disk_car);
  @ requires valid_disk(disk_car);
  @ requires \valid((char *)buffer + (0 .. count-1));
  @ requires separation: \separated(disk_car, (char *)buffer + (0 .. count-1));
  @ decreases 0;
  @*/
static auto io_redir_pread(disk_t &disk_car, void *buffer, const unsigned int count, const uint64_t offset) -> int;

/*@
  @ requires \valid(disk_car);
  @ requires valid_disk(disk_car);
  @ decreases 0;
  @*/
static void io_redir_clean(disk_t &disk_car);

auto io_redir_add_redir(disk_t &disk_car, const uint64_t org_offset, const unsigned int size, const uint64_t new_offset,
                       const void *mem) -> int
{
    if (disk_car.pread != &io_redir_pread)
    {
        auto *data = new struct info_io_redir;
        disk_t old_disk_car = disk_car;
#ifdef DEBUG_IO_REDIR
        log_trace("io_redir_add_redir: install functions org_offset={}, size={}, new_offset={}, mem={:p}",
                  (long long unsigned)org_offset, size, (long long unsigned)new_offset, mem);
#endif
        data->disk_car = &old_disk_car;
        data->list_redir = nullptr;
        disk_car.write_used = 0;
        disk_car.data = data;
        disk_car.description = old_disk_car.description;
        disk_car.pwrite = old_disk_car.pwrite;
        disk_car.pread = &io_redir_pread;
        disk_car.clean = &io_redir_clean;
    }
    {
        auto *data = static_cast<struct info_io_redir *>(disk_car.data);
        list_redir_t *prev_redir = nullptr;
        list_redir_t *current_redir;
        for (current_redir = data->list_redir;
             (current_redir != nullptr) && org_offset < current_redir->org_offset + current_redir->size;
             current_redir = current_redir->next)
            prev_redir = current_redir;
        if (current_redir != nullptr && org_offset >= current_redir->org_offset)
        {
            log_critical("io_redir_add_redir failed: already redirected\n");
            return 1;
        }
        {
            list_redir_t *new_redir;
#ifdef DEBUG_IO_REDIR
            log_trace("io_redir_add_redir: add redirection\n");
#endif
            new_redir = new list_redir_t;
            new_redir->org_offset = org_offset;
            new_redir->size = size;
            new_redir->new_offset = new_offset;
            new_redir->mem = mem;
            new_redir->next = current_redir;
            if (prev_redir != nullptr)
                prev_redir->next = new_redir;
            else
                data->list_redir = new_redir;
        }
    }
    return 0;
}

auto io_redir_del_redir(disk_t &disk_car, uint64_t org_offset) -> int
{
    if (disk_car.pread != &io_redir_pread)
    {
        log_critical("io_redir_del_redir: BUG, no redirection present.\n");
        return 1;
    }
    {
        auto *data = static_cast<struct info_io_redir *>(disk_car.data);
        list_redir_t *current_redir;
        for (current_redir = data->list_redir; (current_redir != nullptr) && org_offset != current_redir->org_offset;
             current_redir = current_redir->next)
            ;
        if (current_redir != nullptr)
        {
#ifdef DEBUG_IO_REDIR
            log_trace("io_redir_del_redir: remove redirection\n");
#endif
            if (current_redir->prev != nullptr)
                current_redir->prev->next = current_redir->next;
            if (current_redir->next != nullptr)
                current_redir->next->prev = current_redir->prev;
            if (data->list_redir == current_redir)
                data->list_redir = current_redir->next;
            delete (current_redir);
            if (data->list_redir == nullptr)
            {
#ifdef DEBUG_IO_REDIR
                log_trace("io_redir_del_redir: uninstall functions\n");
#endif
                disk_car = *data->disk_car;
                delete (data->disk_car);
                delete (data);
            }
            return 0;
        }
        log_critical("io_redir_del_redir: redirection not found\n");
        return 1;
    }
}

static auto io_redir_pread(disk_t &disk_car, void *buffer, const unsigned int count, const uint64_t offset) -> int
{
    auto *data = static_cast<struct info_io_redir *>(disk_car.data);
    uint64_t current_offset = offset;
    unsigned int current_count = count;
    list_redir_t *current_redir;
#ifdef DEBUG_IO_REDIR
    log_trace("io_redir_pread: count={} offset={}", count, (long long unsigned)offset);
#endif
    while (current_count != 0)
    {
        unsigned int read_size;
        int res = 0;
        for (current_redir = data->list_redir;
             (current_redir != nullptr) &&
             !(current_redir->org_offset <= offset && offset < current_redir->org_offset + current_redir->size);
             current_redir = current_redir->next)
            ;
        if (current_redir != nullptr)
        {
            if (current_redir->org_offset > current_offset)
            {
                /* Read data before redirection */
                read_size = current_redir->org_offset - current_offset;
#ifdef DEBUG_IO_REDIR
                log_trace("io_redir_pread: read {} bytes before redirection\n", read_size);
#endif
                res = data->disk_car->pread(*data->disk_car, buffer, read_size, current_offset);
                current_count -= read_size;
                current_offset += read_size;
                buffer = static_cast<unsigned char *>(buffer) + read_size;
            }
            /* Redirection */
            read_size = (current_count > current_redir->size ? current_redir->size : current_count);
            if (current_redir->mem != nullptr)
            {
#ifdef DEBUG_IO_REDIR
                log_trace("io_redir_pread: copy {} bytes from memory\n", read_size);
#endif
                memcpy(buffer, static_cast<const unsigned char *>(current_redir->mem) + current_offset - current_redir->org_offset,
                       read_size);
                res = read_size;
            }
            else
            {
#ifdef DEBUG_IO_REDIR
                log_trace("io_redir_pread: read {} from another position\n", read_size);
#endif
                res = data->disk_car->pread(*data->disk_car, buffer, read_size,
                                            current_redir->new_offset + current_offset - current_redir->org_offset);
                ;
            }
        }
        else
        {
            read_size = current_count;
#ifdef DEBUG_IO_REDIR
            log_trace("io_redir_pread: normal read of {} bytes\n", read_size);
#endif
            res = data->disk_car->pread(*data->disk_car, buffer, read_size, current_offset);
        }
        if (std::cmp_not_equal(res, read_size))
            return res;
        current_count -= read_size;
        current_offset += read_size;
        buffer = static_cast<unsigned char *>(buffer) + read_size;
    }
    return count;
}

static void io_redir_clean(disk_t &disk_car)
{
    if (disk_car.data)
    {
        auto *data = static_cast<struct info_io_redir *>(disk_car.data);
        data->disk_car->clean(*data->disk_car);
        delete (data->disk_car);
        delete static_cast<struct info_io_redir *>(disk_car.data);
        disk_car.data = nullptr;
    }
}
