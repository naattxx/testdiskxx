/*
 * ntfs_io.c - Unix style disk io functions. Part of the TestDisk project.
 *
 * Original version comes from the Linux-NTFS project.
 * Copyright (c) 2000-2003 Anton Altaparmakov
 * Copyright (c) 2004-2006 Christophe Grenier
 *
 * This program/include file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program/include file is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (in the main directory of the Linux-NTFS
 * distribution in the file COPYING); if not, write to the Free Software
 * Foundation,Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <config.h>

#ifdef DISABLED_FOR_FRAMAC
#undef HAVE_LIBNTFS
#undef HAVE_LIBNTFS3G
#endif

#if (defined(HAVE_LIBNTFS) || defined(HAVE_LIBNTFS3G))
#include <cerrno>
#include <cstdlib>
#include <cstring>
#if __has_include(<sys/stat.h>)
#include <sys/stat.h>
#endif
#if __has_include(<fcntl.h>)
#include <fcntl.h>
#endif
#if __has_include(<sys/ioctl.h>)
#include <sys/ioctl.h>
#endif
#include <cstdarg>
#ifdef HAVE_LIBNTFS
#include <ntfs/device.h>
#endif
#ifdef HAVE_LIBNTFS3G
#define HAVE_SYS_TYPES_H
#define HAVE_SYS_STAT_H
#define HAVE_TIME_H
#define HAVE_STDDEF_H
extern "C"
{
#include <ntfs-3g/device.h>
#include <ntfs-3g/types.h>
}
#undef min
#undef max
#endif
#include <cstdio>
// #include "types.h"
#include "src/common.hpp"
#include "src/log.hpp"

#if defined(linux) && defined(_IO) && !defined(BLKGETSIZE)
#define BLKGETSIZE _IO(0x12, 96) /* Get device size in 512byte blocks. */
#endif

static auto ntfs_device_testdisk_io_open(struct ntfs_device *dev, int flags)
    -> int
{
  if (NDevOpen(dev))
  {
    errno = EBUSY;
    return -1;
  }
  /* Setup our read-only flag. */
  if ((flags & O_RDWR) != O_RDWR)
    NDevSetReadOnly(dev);
  /* Set our open flag. */
  NDevSetOpen(dev);
  return 0;
}

static auto ntfs_device_testdisk_io_close(struct ntfs_device *dev) -> int
{
  if (!NDevOpen(dev))
  {
    errno = EBADF;
    return -1;
  }
  NDevClearOpen(dev);
  return 0;
}

static auto ntfs_device_testdisk_io_seek(struct ntfs_device *dev, s64 offset,
                                         int whence) -> s64
{
  auto *my_data = static_cast<my_data_t *>(dev->d_private);
  switch (whence)
  {
  case SEEK_SET:
    my_data->offset = offset;
    break;
  case SEEK_CUR:
    my_data->offset += offset;
    break;
  case SEEK_END:
    my_data->offset = my_data->partition.part_size + offset;
    break;
  }
  return my_data->offset;
}

static auto ntfs_device_testdisk_io_read(struct ntfs_device *dev, void *buf,
                                         s64 count) -> s64
{
  auto *my_data = static_cast<my_data_t *>(dev->d_private);
  if (my_data->disk_car->pread(*my_data->disk_car, buf, count,
                               my_data->partition.part_offset +
                                   my_data->offset) != count)
    return 0;
  my_data->offset += count;
  return count;
}

static auto ntfs_device_testdisk_io_write(struct ntfs_device *dev,
                                          const void *buf, s64 count) -> s64
{
  auto *my_data = static_cast<my_data_t *>(dev->d_private);
  if (my_data->disk_car->pwrite(*my_data->disk_car, buf, count,
                                my_data->partition.part_offset +
                                    my_data->offset) != count)
    return 0;
  my_data->offset += count;
  return count;
}

static auto ntfs_device_testdisk_io_pread(struct ntfs_device *dev, void *buf,
                                          s64 count, s64 offset) -> s64
{
  auto *my_data = static_cast<my_data_t *>(dev->d_private);
  return my_data->disk_car->pread(*my_data->disk_car, buf, count,
                                  my_data->partition.part_offset + offset);
}

static auto ntfs_device_testdisk_io_pwrite(struct ntfs_device *dev,
                                           const void *buf, s64 count,
                                           s64 offset) -> s64
{
  auto *my_data = static_cast<my_data_t *>(dev->d_private);
  return my_data->disk_car->pwrite(*my_data->disk_car, buf, count,
                                   my_data->partition.part_offset + offset);
}

static auto ntfs_device_testdisk_io_sync(struct ntfs_device *dev) -> int
{
  auto *my_data = static_cast<my_data_t *>(dev->d_private);
  return my_data->disk_car->sync(*my_data->disk_car);
}

static auto ntfs_device_testdisk_io_stat(struct ntfs_device *dev,
                                         struct stat *buf) -> int
{
  log_warning("ntfs_device_testdisk_io_stat() unimplemented\n");
#ifdef ENOTSUP
  errno = ENOTSUP;
#endif
  return -1;
}

#ifdef NTFS_DEVICE_OPERATIONS_IOCTL_INT
static int ntfs_device_testdisk_io_ioctl(struct ntfs_device *dev, int request,
                                         void *argp)
{
  log_warning("ntfs_device_testdisk_io_ioctl() unimplemented\n");
#ifdef ENOTSUP
  errno = ENOTSUP;
#endif
  return -1;
}
#elifdef NTFS_DEVICE_OPERATIONS_IOCTL_UL
static auto ntfs_device_testdisk_io_ioctl(struct ntfs_device *dev,
                                          unsigned long request, void *argp)
    -> int
{
  log_warning("ntfs_device_testdisk_io_ioctl() unimplemented\n");
#ifdef ENOTSUP
  errno = ENOTSUP;
#endif
  return -1;
}
#endif

/**
 * Device operations for working with unix style devices and files.
 */
struct ntfs_device_operations ntfs_device_testdisk_io_ops = {
    .open   = &ntfs_device_testdisk_io_open,
    .close  = &ntfs_device_testdisk_io_close,
    .seek   = &ntfs_device_testdisk_io_seek,
    .read   = &ntfs_device_testdisk_io_read,
    .write  = &ntfs_device_testdisk_io_write,
    .pread  = &ntfs_device_testdisk_io_pread,
    .pwrite = &ntfs_device_testdisk_io_pwrite,
    .sync   = &ntfs_device_testdisk_io_sync,
    .stat   = &ntfs_device_testdisk_io_stat,
    .ioctl  = &ntfs_device_testdisk_io_ioctl,
};
#endif
