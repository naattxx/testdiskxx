/*

    File: ext2_dir.c

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
#include "src/dir_common.hpp"
#include <config.h>
#include <utility>

#if defined(DISABLED_FOR_FRAMAC)
#undef HAVE_LIBEXT2FS
#endif

#include <cerrno>
#include <cstdio>
#include <cstring>

#if defined(HAVE_LIBEXT2FS)
#if __has_include("ext2fs/ext2_fs.h")
#include "ext2fs/ext2_fs.h"
#endif
#if __has_include("ext2fs/ext2fs.h")
#include "ext2fs/ext2fs.h"
#endif
#undef clamp
#endif

// #include "types.h"
#include "ext2_dir.hpp"
#include "src/common.hpp"
#include "src/dir.hpp"
#include "src/intrf.hpp"
#include "ext2_inc.hpp"
#include "src/log.hpp"
#include "src/setdate.hpp"

#if defined(HAVE_LIBEXT2FS)
#define DIRENT_DELETED_FILE 4
/*
 * list directory
 */

#define LONG_OPT 0x0001

/*
 * I/O Manager routine prototypes
 */
static auto my_open(const char *dev, int flags, io_channel *channel) -> errcode_t;
static auto my_close(io_channel channel) -> errcode_t;
static auto my_set_blksize(io_channel channel, int blksize) -> errcode_t;
static auto my_read_blk(io_channel channel, unsigned long block, int count, void *buf) -> errcode_t;
static auto my_write_blk(io_channel channel, unsigned long block, int count, const void *buf) -> errcode_t;
static auto my_flush(io_channel channel) -> errcode_t;
static auto my_read_blk64(io_channel channel, unsigned long long block, int count, void *buf) -> errcode_t;
static auto my_write_blk64(io_channel channel, unsigned long long block, int count, const void *buf) -> errcode_t;

static void dir_partition_ext2_close(dir_data_t *dir_data);
static auto ext2_copy(disk_t &disk_car, const partition_t &partition, dir_data_t *dir_data, const file_info_t *file)
    -> copy_file_t;

static struct struct_io_manager my_struct_manager = {
    .magic = EXT2_ET_MAGIC_IO_MANAGER,
    .name = "TestDisk I/O Manager",
    .open = &my_open,
    .close = &my_close,
    .set_blksize = &my_set_blksize,
    .read_blk = &my_read_blk,
    .write_blk = &my_write_blk,
    .flush = &my_flush,
    .write_byte = nullptr,
#ifdef HAVE_STRUCT_STRUCT_IO_MANAGER_SET_OPTION
    .set_option = NULL,
#endif
#ifdef HAVE_STRUCT_STRUCT_IO_MANAGER_READ_BLK64
    .read_blk64 = &my_read_blk64,
#endif
#ifdef HAVE_STRUCT_STRUCT_IO_MANAGER_WRITE_BLK64
    .write_blk64 = &my_write_blk64,
#endif
};

static io_channel shared_ioch = nullptr;
/*
 * Macro taken from unix_io.c
 * For checking structure magic numbers...
 */

#define EXT2_CHECK_MAGIC(struct, code)                                                                                 \
    if ((struct)->magic != (code))                                                                                     \
    return (code)

/*
 * Allocate libext2fs structures associated with I/O manager
 */
static auto alloc_io_channel(const disk_t &disk_car, my_data_t *my_data) -> io_channel
{
    io_channel ioch;
#ifdef DEBUG_EXT2
    log_info("alloc_io_channel start\n");
#endif
    ioch = static_cast<io_channel>(new struct struct_io_channel);
    if (ioch == nullptr)
        return nullptr;
    memset(ioch, 0, sizeof(struct struct_io_channel));
    ioch->magic = EXT2_ET_MAGIC_IO_CHANNEL;
    ioch->manager = &my_struct_manager;
    ioch->name = strdup(my_data->partition.fsname);
    if (ioch->name == nullptr)
    {
        delete (ioch);
        return nullptr;
    }
    ioch->private_data = my_data;
    ioch->block_size = 1024; /* The smallest ext2fs block size */
    ioch->read_error = nullptr;
    ioch->write_error = nullptr;
#ifdef DEBUG_EXT2
    log_info("alloc_io_channel end\n");
#endif
    return ioch;
}

static auto my_open(const char *dev, int flags, io_channel *channel) -> errcode_t
{
    *channel = shared_ioch;
#ifdef DEBUG_EXT2
    log_info("my_open {} done\n", dev);
#endif
    return 0;
}

static auto my_close(io_channel channel) -> errcode_t
{
    delete static_cast<my_data_t *>(channel->private_data);
    delete (channel->name);
    delete (channel);
#ifdef DEBUG_EXT2
    log_info("my_close done\n");
#endif
    return 0;
}

static auto my_set_blksize(io_channel channel, int blksize) -> errcode_t
{
    channel->block_size = blksize;
#ifdef DEBUG_EXT2
    log_info("my_set_blksize done\n");
#endif
    return 0;
}

static auto my_read_blk64(io_channel channel, unsigned long long block, int count, void *buf) -> errcode_t
{
    ssize_t size;
    const auto *my_data = static_cast<const my_data_t *>(channel->private_data);
    EXT2_CHECK_MAGIC(channel, EXT2_ET_MAGIC_IO_CHANNEL);

    size = (count < 0) ? -count : count * channel->block_size;
#ifdef DEBUG_EXT2
    log_info("my_read_blk start size={}, offset={} name={}, block={}, count={}, buf={:p}", (long unsigned)size,
             (unsigned long)(block * channel->block_size), my_data->partition.fsname, block, count, buf);
#endif
    if (my_data->disk_car->pread(*my_data->disk_car, buf, size,
                                 my_data->partition.part_offset + static_cast<uint64_t>(block) * channel->block_size) !=
        size)
        return 1;
#ifdef DEBUG_EXT2
    log_info("my_read_blk done\n");
#endif
    return 0;
}

static auto my_read_blk(io_channel channel, unsigned long block, int count, void *buf) -> errcode_t
{
    return my_read_blk64(channel, block, count, buf);
}

static auto my_write_blk64(io_channel channel, unsigned long long block, int count, const void *buf) -> errcode_t
{
    EXT2_CHECK_MAGIC(channel, EXT2_ET_MAGIC_IO_CHANNEL);
#if 1
    {
        const auto *my_data = reinterpret_cast<const my_data_t *>(channel);
        if (my_data->disk_car->pwrite(*my_data->disk_car, buf, count * channel->block_size,
                                      my_data->partition.part_offset +
                                          static_cast<uint64_t>(block) * channel->block_size) !=
            count * channel->block_size)
            return 1;
        return 0;
    }
#else
    return 1;
#endif
}

static auto my_write_blk(io_channel channel, unsigned long block, int count, const void *buf) -> errcode_t
{
    return my_write_blk64(channel, block, count, buf);
}

static auto my_flush(io_channel channel) -> errcode_t
{
    return 0;
}

static auto list_dir_proc2(ext2_ino_t dir, int entry, struct ext2_dir_entry *dirent, int offset, int blocksize,
                           char *buf, void *privateinfo) -> int
{
    struct ext2_inode inode;
    ext2_ino_t ino;
    auto *ls = static_cast<struct ext2_dir_struct *>(privateinfo);
    file_info_t new_file;
    errcode_t retval;
    if (entry == DIRENT_DELETED_FILE && (ls->dir_data->param & FLAG_LIST_DELETED) == 0)
        return 0;
    ino = dirent->inode;
    if (ino == 0)
        return 0;
    if ((retval = ext2fs_read_inode(ls->current_fs, ino, &inode)) != 0)
    {
        log_error("ext2fs_read_inode(ino={}) failed with error {}.", (unsigned)ino, (long)retval);
        return 0;
    }
    if (inode.i_mode == 0)
        return 0;
    {
        const unsigned int thislen =
            ((dirent->name_len & 0xFF) < EXT2_NAME_LEN) ? (dirent->name_len & 0xFF) : EXT2_NAME_LEN;
        new_file.name = new char[thislen + 1];
        memcpy(new_file.name, dirent->name, thislen);
        new_file.name[thislen] = '\0';
    }
    if (entry == DIRENT_DELETED_FILE)
        new_file.status = FILE_STATUS_DELETED;
    else
        new_file.status = 0;
    new_file.st_ino = ino;
    new_file.st_mode = inode.i_mode;
    //  new_file->st_nlink=inode.i_links_count;
    new_file.st_uid = inode.i_uid;
    new_file.st_gid = inode.i_gid;
    new_file.st_size =
        LINUX_S_ISDIR(inode.i_mode) ? inode.i_size : inode.i_size | (static_cast<uint64_t>(inode.i_size_high) << 32);
    //  new_file->st_blksize=blocksize;
    //  new_file->st_blocks=inode.i_blocks;
    new_file.td_atime = inode.i_atime;
    new_file.td_mtime = inode.i_mtime;
    new_file.td_ctime = inode.i_ctime;
    ls->dir_list.push_front(new_file);
    return 0;
}

static auto ext2_dir(disk_t &disk_car, const partition_t &partition, dir_data_t *dir_data,
                     const unsigned long int cluster, dir_list_t &dir_list) -> int
{
    errcode_t retval;
    auto *ls = static_cast<struct ext2_dir_struct *>(dir_data->private_dir_data);
    ls->dir_list = dir_list;
    if ((retval = ext2fs_dir_iterate2(ls->current_fs, cluster, ls->flags, nullptr, list_dir_proc2, ls)) != 0)
    {
        log_error("ext2fs_dir_iterate failed with error {}.", (long)retval);
        return -1;
    }
    return 0;
}

static void dir_partition_ext2_close(dir_data_t *dir_data)
{
    auto *ls = static_cast<struct ext2_dir_struct *>(dir_data->private_dir_data);
    ext2fs_close(ls->current_fs);
    /* ext2fs_close call the close function that freed my_data */
    delete (ls);
}

static auto ext2_copy(disk_t &disk_car, const partition_t &partition, dir_data_t *dir_data, const file_info_t &file)
    -> copy_file_t
{
    copy_file_t error = CP_OK;
    FILE *f_out;
    const auto *ls = static_cast<const struct ext2_dir_struct *>(dir_data->private_dir_data);
    char *new_file;
    f_out = fopen_local(&new_file, dir_data->local_dir, dir_data->current_directory);
    if (!f_out)
    {
        log_critical("Can't create file %s: %s\n", new_file, strerror(errno));
        delete (new_file);
        return CP_CREATE_FAILED;
    }
    {
        errcode_t retval;
        struct ext2_inode inode;
        char buffer[8192];
        ext2_file_t e2_file;

        if (ext2fs_read_inode(ls->current_fs, file.st_ino, &inode) != 0)
        {
            delete (new_file);
            fclose(f_out);
            return CP_STAT_FAILED;
        }

        retval = ext2fs_file_open(ls->current_fs, file.st_ino, 0, &e2_file);
        if (retval)
        {
            log_error("Error while opening ext2 file %s\n", dir_data->current_directory);
            delete (new_file);
            fclose(f_out);
            return CP_OPEN_FAILED;
        }
        while (error != CP_NOSPACE)
        {
            int nbytes;
            unsigned int got;
            retval = ext2fs_file_read(e2_file, buffer, sizeof(buffer), &got);
            if (retval)
            {
                log_error("Error while reading ext2 file %s\n", dir_data->current_directory);
                error = CP_READ_FAILED;
            }
            if (got == 0)
                break;
            nbytes = fwrite(buffer, 1, got, f_out);
            if (std::cmp_not_equal(nbytes, got))
            {
                log_error("Error while writing file %s\n", new_file);
                error = CP_NOSPACE;
            }
        }
        retval = ext2fs_file_close(e2_file);
        if (retval)
        {
            log_error("Error while closing ext2 file\n");
            error = CP_CLOSE_FAILED;
        }
        fclose(f_out);
        set_date(new_file, file.td_atime, file.td_mtime);
        (void)set_mode(new_file, file.st_mode);
    }
    delete (new_file);
    return error;
}
#endif

auto dir_partition_ext2_init(disk_t &disk_car, const partition_t &partition, dir_data_t *dir_data, const int verbose)
    -> dir_partition_t
{
#if defined(HAVE_LIBEXT2FS)
    auto *ls = new struct ext2_dir_struct;
    io_channel ioch;
    my_data_t *my_data;
    /*  ls->flags = DIRENT_FLAG_INCLUDE_EMPTY; */
    ls->flags = DIRENT_FLAG_INCLUDE_REMOVED;
    ls->dir_data = dir_data;
    my_data = reinterpret_cast<my_data_t *>(new unsigned char[sizeof(*my_data)]);
    my_data->partition = partition;
    my_data->disk_car = &disk_car;
    ioch = alloc_io_channel(disk_car, my_data);
    shared_ioch = ioch;
    /* An alternate superblock may be used if the calling function has set an IO redirection */
    if (ext2fs_open("/dev/testdisk", 0, 0, 0, &my_struct_manager, &ls->current_fs) != 0)
    {
        //    delete (my_data);
        delete (ls);
        return DIR_PART_EIO;
    }
    strncpy(dir_data->current_directory, "/", sizeof(dir_data->current_directory));
    dir_data->current_inode = EXT2_ROOT_INO;
    dir_data->param = FLAG_LIST_DELETED;
    dir_data->verbose = verbose;
    dir_data->capabilities = CAPA_LIST_DELETED;
    dir_data->get_dir = &ext2_dir;
    dir_data->copy_file = &ext2_copy;
    dir_data->close = &dir_partition_ext2_close;
    dir_data->local_dir = nullptr;
    dir_data->private_dir_data = ls;
    return DIR_PART_OK;
#else
    return DIR_PART_ENOSYS;
#endif
}

auto td_ext2fs_version() -> const char *
{
    const char *ext2fs_version = "none";
#if defined(HAVE_LIBEXT2FS)
    ext2fs_get_library_version(&ext2fs_version, nullptr);
#endif
    return ext2fs_version;
}
