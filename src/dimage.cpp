/*

    File: dimage.c

    Copyright (C) 2007-2009 Christophe GRENIER <grenier@cgsecurity.org>

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

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string_view>
#if __has_include(<sys/stat.h>)
#include <sys/stat.h>
#endif
#if __has_include(<sys/types.h>)
#include <sys/types.h>
#endif
#if __has_include(<unistd.h>)
#include <unistd.h>
#endif
#include <cassert>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <utility>
#include "common.hpp"
#include "dimage.hpp"
#include "log.hpp"

#define READ_SIZE 256 * 512
/* Skip 10Mb when there is a read error */
#define SKIP_SIZE 10 * 1024 * 1024

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif
#ifndef O_BINARY
#define O_BINARY 0
#endif

static void disk_image_backward(int disk_dst, disk_t &disk,
                                const uint64_t src_offset_start,
                                const uint64_t src_offset_end,
                                uint64_t dst_offset)
{
  uint64_t src_offset;
  auto *buffer = new unsigned char[disk.sector_size];
  for (src_offset = src_offset_end - disk.sector_size;
       src_offset > src_offset_start;
       src_offset -= disk.sector_size, dst_offset -= disk.sector_size)
  {
    const ssize_t pread_res =
        disk.pread(disk, buffer, disk.sector_size, src_offset);
    if (std::cmp_not_equal(pread_res, disk.sector_size))
    {
      delete[] buffer;
      return;
    }
#ifdef HAVE_PWRITE
    if (pwrite(disk_dst, buffer, pread_res, src_offset) < 0)
    {
      delete[] buffer;
      return;
    }
#else
    if (lseek(disk_dst, src_offset, SEEK_SET) < 0)
    {
      delete[] buffer;
      return;
    }
    if (write(disk_dst, buffer, pread_res) != pread_res)
    {
      delete[] buffer;
      return;
    }
#endif
  }
  delete[] buffer;
}

auto disk_image(disk_t &disk, const partition_t &partition,
                const std::filesystem::path &image_dd,
                float *out_percent, bool &in_stop) -> std::string_view
{
  *out_percent = 0.f;
  int ind_stop            = 0;
  uint64_t nbr_read_error = 0;
  uint64_t src_offset     = partition.part_offset;
  uint64_t src_offset_old;
  uint64_t dst_offset           = 0;
  const uint64_t src_offset_end = partition.part_offset + partition.part_size;
  const uint64_t offset_inc     = (src_offset_end - src_offset) / 10000;
  uint64_t src_offset_next      = src_offset;
  auto *buffer          = new unsigned char[READ_SIZE];
  unsigned int readsize = READ_SIZE;
  int disk_dst;
#ifdef HAVE_PWRITE
  int use_pwrite = 1;
#endif
  assert(disk.sector_size > 0);
  assert(disk.sector_size <= READ_SIZE);
  if ((disk_dst = open(image_dd.c_str(),
                       O_CREAT | O_LARGEFILE | O_RDWR | O_BINARY, 0644)) < 0)
  {
    log_error("Can't create file {}.", image_dd.string());
    delete[] buffer;
    return "Can't create file!";
  }
  src_offset_old = src_offset;

  while (ind_stop == 0 && !in_stop && src_offset < src_offset_end)
  {
    ssize_t pread_res;
    int update = 0;
    readsize   = std::min<uint64_t>(src_offset_end - src_offset, readsize);
    pread_res  = disk.pread(disk, buffer, readsize, src_offset);
    if (pread_res > 0)
    {
#ifdef HAVE_PWRITE
      if (use_pwrite > 0 && pwrite(disk_dst, buffer, pread_res, dst_offset) < 0)
#endif
      {
#ifdef HAVE_PWRITE
        use_pwrite = 0;
#endif
        if (lseek(disk_dst, dst_offset, SEEK_SET) < 0)
        {
          ind_stop = 2;
          log_critical("disk_image lseek() failed: {}\n", strerror(errno));
        }
        else if (write(disk_dst, buffer, pread_res) != pread_res)
        {
          log_critical("disk_image write() failed: {}\n", strerror(errno));
          ind_stop = 2;
        }
      }
      if (src_offset_old + SKIP_SIZE == src_offset)
      {
        disk_image_backward(disk_dst, disk, src_offset_old, src_offset,
                            dst_offset);
      }
    }
    src_offset_old = src_offset;
    if (std::cmp_equal(pread_res, readsize))
    {
      src_offset += readsize;
      dst_offset += readsize;
      readsize = READ_SIZE;
    }
    else
    {
      update = 1;
      nbr_read_error++;
      readsize = disk.sector_size;
      src_offset += SKIP_SIZE;
      dst_offset += SKIP_SIZE;
    }
    if (src_offset > src_offset_next)
    {
      update          = 1;
      src_offset_next = src_offset + offset_inc;
    }
    if (update && ind_stop == 0 && !in_stop)
    {
      *out_percent = static_cast<float>(src_offset - partition.part_offset) / partition.part_size;
    }
  }
  close(disk_dst);
  delete[] buffer;
  if (ind_stop == 2)
    return "No space left for the file image.";
  if (ind_stop || in_stop)
  {
    if (nbr_read_error == 0)
      return "Incomplete image created";
    return "Incomplete image created: read errors have occured.";
  }
  if (nbr_read_error == 0)
    return "Image created successfully.";
  return "Image created successfully but read errors have occured.";
}
