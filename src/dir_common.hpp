/*

    File: dir_common.h

    Copyright (C) 2020 Christophe GRENIER <grenier@cgsecurity.org>

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
#ifndef _DIR_COMMON_H
#define _DIR_COMMON_H
#include <cstdint>
#include <filesystem>
#include <string>
#if __has_include(<sys/stat.h>)
#include <sys/stat.h>
#endif
#include "common.hpp"
#define DIR_NAME_LEN 1024u
enum FLAG_LIST : uint8_t {
  DELETED = 1,
  MASK12 = 2,
  MASK16 = 4,
  PATHNAME = 8,
  ADS = 16,
  SYSTEM = 32,
};
/* capabilities */
#define CAPA_LIST_DELETED 1
#define CAPA_LIST_ADS 2

enum copy_file_t : int8_t
{
    CP_OK = 0,
    CP_STAT_FAILED = -1,
    CP_OPEN_FAILED = -2,
    CP_READ_FAILED = -3,
    CP_CREATE_FAILED = -4,
    CP_NOSPACE = -5,
    CP_CLOSE_FAILED = -6,
    CP_NOMEM         = -7,
};
enum dir_partition_t : int8_t
{
    DIR_PART_ENOIMP = -3,
    DIR_PART_ENOSYS = -2,
    DIR_PART_EIO = -1,
    DIR_PART_OK     = 0,
};

struct file_status {
  bool deleted : 1;
  bool marked : 1;
  bool ads : 1;
};

struct file_info_t
{
    std::string name;
    uint32_t st_ino;
    uint32_t st_mode;
    uint32_t st_uid;
    uint32_t st_gid;
    uint64_t st_size;
    time_t td_atime; /* time of last access */
    time_t td_mtime; /* time of last modification */
    time_t td_ctime; /* time of last status change */
    file_status status;
};
using dir_list_t = std::list<file_info_t>;

struct dir_data_t
{
    char current_directory[DIR_NAME_LEN];
    unsigned long int current_inode;
    int verbose;
    unsigned int param;
    unsigned int capabilities;
    virtual auto get_dir(disk_t &disk_car, const partition_t &partition,
                         const unsigned long int first_inode, dir_list_t &list)
        -> int                                                     = 0;
    virtual auto copy_file(disk_t &disk_car, const partition_t &partition,
                           const file_info_t &file) -> copy_file_t = 0;
    virtual void close()                                           = 0;
    virtual ~dir_data_t()                                          = default;
    std::filesystem::path local_dir;
};

constexpr unsigned LINUX_S_IFMT   = 00170000;
constexpr unsigned LINUX_S_IFSOCK = 0140000;
constexpr unsigned LINUX_S_IFLNK  = 0120000;
constexpr unsigned LINUX_S_IFREG  = 0100000;
constexpr unsigned LINUX_S_IFBLK  = 0060000;
constexpr unsigned LINUX_S_IFDIR  = 0040000;
constexpr unsigned LINUX_S_IFCHR  = 0020000;
constexpr unsigned LINUX_S_IFIFO  = 0010000;
constexpr unsigned LINUX_S_ISUID  = 0004000;
constexpr unsigned LINUX_S_ISGID  = 0002000;
constexpr unsigned LINUX_S_ISVTX  = 0001000;

constexpr unsigned LINUX_S_IRWXU = 00700;
constexpr unsigned LINUX_S_IRUSR = 00400;
constexpr unsigned LINUX_S_IWUSR = 00200;
constexpr unsigned LINUX_S_IXUSR = 00100;

constexpr unsigned LINUX_S_IRWXG = 00070;
constexpr unsigned LINUX_S_IRGRP = 00040;
constexpr unsigned LINUX_S_IWGRP = 00020;
constexpr unsigned LINUX_S_IXGRP = 00010;

constexpr unsigned LINUX_S_IRWXO = 00007;
constexpr unsigned LINUX_S_IROTH = 00004;
constexpr unsigned LINUX_S_IWOTH = 00002;
constexpr unsigned LINUX_S_IXOTH = 00001;

constexpr unsigned LINUX_S_IRWXUGO {LINUX_S_IRWXU | LINUX_S_IRWXG | LINUX_S_IRWXO};
constexpr unsigned LINUX_S_IALLUGO {LINUX_S_ISUID | LINUX_S_ISGID | LINUX_S_ISVTX | LINUX_S_IRWXUGO};
constexpr unsigned LINUX_S_IRUGO {LINUX_S_IRUSR | LINUX_S_IRGRP | LINUX_S_IROTH};
constexpr unsigned LINUX_S_IWUGO {LINUX_S_IWUSR | LINUX_S_IWGRP | LINUX_S_IWOTH};
constexpr unsigned LINUX_S_IXUGO {LINUX_S_IXUSR | LINUX_S_IXGRP | LINUX_S_IXOTH};

#define LINUX_S_ISLNK(m) (((m) & LINUX_S_IFMT) == LINUX_S_IFLNK)
#define LINUX_S_ISREG(m) (((m) & LINUX_S_IFMT) == LINUX_S_IFREG)
#define LINUX_S_ISDIR(m) (((m) & LINUX_S_IFMT) == LINUX_S_IFDIR)
#define LINUX_S_ISCHR(m) (((m) & LINUX_S_IFMT) == LINUX_S_IFCHR)
#define LINUX_S_ISBLK(m) (((m) & LINUX_S_IFMT) == LINUX_S_IFBLK)
#define LINUX_S_ISFIFO(m) (((m) & LINUX_S_IFMT) == LINUX_S_IFIFO)
#define LINUX_S_ISSOCK(m) (((m) & LINUX_S_IFMT) == LINUX_S_IFSOCK)

#endif
