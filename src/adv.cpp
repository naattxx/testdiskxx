/*

    File: adv.c

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
#include <config.h>

#include <cassert>
#include <cstddef>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
// #include "types.h"
#include "common.hpp"
#include "intrf.hpp"
// #include "intrfn.h"
#include "fnctdsk.hpp"
// #include "chgtypen.h"
#include "addpart.hpp"
#include "adv.hpp"
#include "askloc.hpp"
#include "dimage.hpp"
#include "dirpart.hpp"
#include "guid_cmp.hpp"
#include "part/ext2_sb.hpp"
#include "part/ext2_sbn.hpp"
#include "part/fat.hpp"
#include "part/fat1x.hpp"
#include "part/fat32.hpp"
#include "part/ntfs.hpp"
#include "part/ntfs_udl.hpp"
#include "part/texfat.hpp"
#include "part/thfs.hpp"
#include "part/tntfs.hpp"
// #include "addpartn.h"
#include "io_redir.hpp"

extern const arch_fnct_t arch_gpt;
extern const arch_fnct_t arch_i386;
extern const arch_fnct_t arch_mac;
extern const arch_fnct_t arch_none;
extern const arch_fnct_t arch_sun;
extern const arch_fnct_t arch_xbox;

#ifdef HAVE_NCURSES
#define INTER_ADV_X 0
#define INTER_ADV_Y (LINES - 2)
#define INTER_ADV (LINES - 2 - 7 - 1)
#endif

#define DEFAULT_IMAGE_NAME "image.dd"

static auto is_part_hfs(const partition_t &partition) -> int
{
    if (partition.part_type_i386 == P_HFS || partition.part_type_mac == PMAC_HFS)
        return 1;
    if (partition.part_type_gpt == GPT_ENT_TYPE_MAC_HFS)
        return 1;
    return 0;
}

static auto is_part_hfsp(const partition_t &partition) -> int
{
    if (partition.part_type_i386 == P_HFSP || partition.part_type_mac == PMAC_HFS)
        return 1;
    if (partition.part_type_gpt == GPT_ENT_TYPE_MAC_HFS)
        return 1;
    return 0;
}

auto is_part_linux(const partition_t &partition) -> int
{
    if (partition.arch == &arch_i386 && partition.part_type_i386 == P_LINUX)
        return 1;
    if (partition.arch == &arch_sun && partition.part_type_sun == PSUN_LINUX)
        return 1;
    if (partition.arch == &arch_mac && partition.part_type_mac == PMAC_LINUX)
        return 1;
    if (partition.arch == &arch_gpt && (partition.part_type_gpt == GPT_ENT_TYPE_LINUX_DATA ||
                                         partition.part_type_gpt == GPT_ENT_TYPE_LINUX_HOME ||
                                         partition.part_type_gpt == GPT_ENT_TYPE_LINUX_SRV))
        return 1;
    return 0;
}

static auto is_exfat(const partition_t &partition) -> int
{
    return (is_part_ntfs(partition) || partition.upart_type == UP_EXFAT);
}

static auto is_hfs(const partition_t &partition) -> int
{
    return (is_part_hfs(partition) || partition.upart_type == UP_HFS);
}

static auto is_hfsp(const partition_t &partition) -> int
{
    return (is_part_hfsp(partition) || partition.upart_type == UP_HFSP || partition.upart_type == UP_HFSX);
}

static auto is_linux(const partition_t &partition) -> int
{
    if (is_part_linux(partition))
        return 1;
    switch (partition.upart_type)
    {
    case UP_CRAMFS:
    case UP_EXT2:
    case UP_EXT3:
    case UP_EXT4:
    case UP_JFS:
    case UP_RFS:
    case UP_RFS2:
    case UP_RFS3:
    case UP_RFS4:
    case UP_XFS:
    case UP_XFS2:
    case UP_XFS3:
    case UP_XFS4:
    case UP_XFS5:
        return 1;
    default:
        break;
    }
    return 0;
}

#ifdef HAVE_NCURSES
static const char *adv_get_boot_description(const partition_t &partition)
{
    assert(partition != NULL);
    if (is_part_linux(partition))
    {
        return "Locate ext2/ext3/ext4 backup superblock";
    }
    else if (is_part_hfs(partition) || is_part_hfsp(partition))
    {
        return "Locate HFS/HFS+ backup volume header";
    }
    else if (is_linux(partition))
    {
        return "Locate ext2/ext3/ext4 backup superblock";
    }
    else if (is_hfs(partition) || is_hfsp(partition))
    {
        return "Locate HFS/HFS+ backup volume header";
    }
    return "Boot sector recovery";
}

static const char *adv_get_options_for_partition(const partition_t &partition)
{
    if (is_part_fat(partition))
    {
        return "tubcq";
    }
    else if (is_part_ntfs(partition))
        return "tlubcq";
    else if (is_part_linux(partition))
    {
        if (partition.upart_type == UP_EXT2)
            return "tuscq";
        else
            return "tlscq";
    }
    else if (is_part_hfs(partition) || is_part_hfsp(partition))
    {
        return "tscq";
    }
    else if (is_fat(partition))
        return "tubcq";
    else if (is_ntfs(partition) || is_exfat(partition))
        return "tlubcq";
    else if (is_linux(partition))
    {
        if (partition.upart_type == UP_EXT2)
            return "tluscq";
        else
            return "tlscq";
    }
    else if (is_hfs(partition) || is_hfsp(partition))
    {
        return "tscq";
    }
    return "tcq";
}
#endif

static auto adv_menu_boot_selected(disk_t &disk, partition_t &partition, const int verbose, const int dump_ind,
                                  const unsigned int expert, char **current_cmd) -> int
{
    if (is_part_fat32(partition))
    {
        fat32_boot_sector(disk, partition, verbose, dump_ind, expert, current_cmd);
        return 1;
    }
    if (is_part_fat12(partition) || is_part_fat16(partition))
    {
      fat1x_boot_sector(disk, partition, verbose, dump_ind, expert,
                        current_cmd);
      return 1;
    }
    if (is_part_ntfs(partition))
    {
      if (partition.upart_type == UP_EXFAT)
        exFAT_boot_sector(disk, partition, current_cmd);
      else
        ntfs_boot_sector(disk, partition, verbose, expert, current_cmd);
      return 1;
    }
    if (partition.upart_type == UP_FAT32)
    {
      fat32_boot_sector(disk, partition, verbose, dump_ind, expert,
                        current_cmd);
      return 1;
    }
    if (partition.upart_type == UP_FAT12 || partition.upart_type == UP_FAT16)
    {
      fat1x_boot_sector(disk, partition, verbose, dump_ind, expert,
                        current_cmd);
      return 1;
    }
    if (partition.upart_type == UP_NTFS)
    {
      ntfs_boot_sector(disk, partition, verbose, expert, current_cmd);
      return 1;
    }
    if (partition.upart_type == UP_EXFAT)
    {
      exFAT_boot_sector(disk, partition, current_cmd);
      return 1;
    }
    return 0;
}

static void adv_menu_image_selected(disk_t &disk, const partition_t &partition, char **current_cmd)
{
    char dst_path[4096];
    dst_path[0] = '\0';
#ifdef HAVE_NCURSES
    if (*current_cmd != NULL)
        td_getcwd(dst_path, sizeof(dst_path));
    else
    {
        char msg[256];
        snprintf(msg, sizeof(msg), "Please select where to store the file image.dd (%u MB), an image of the partition",
                 (unsigned int)(partition.part_size / 1000 / 1000));
        ask_location(dst_path, sizeof(dst_path), msg, "");
    }
#else
    td_getcwd(dst_path, sizeof(dst_path));
#endif
    if (dst_path[0] != '\0')
    {
        char *filename = new char[strlen(dst_path) + 1 + strlen(DEFAULT_IMAGE_NAME) + 1];
        strcpy(filename, dst_path);
        strcat(filename, "/");
        strcat(filename, DEFAULT_IMAGE_NAME);
        disk_image(disk, partition, filename);
        delete[] filename;
    }
}

static void adv_menu_undelete_selected(disk_t &disk, const partition_t &partition, const int verbose,
                                       char **current_cmd)
{
    if (partition.sb_offset != 0 && partition.sb_size > 0)
    {
        io_redir_add_redir(disk, partition.part_offset + partition.sborg_offset, partition.sb_size,
                           partition.part_offset + partition.sb_offset, nullptr);
        if (partition.upart_type == UP_NTFS || (is_part_ntfs(partition) && partition.upart_type != UP_EXFAT))
            ntfs_undelete_part(disk, partition, verbose, current_cmd);
        else
            dir_partition(disk, partition, 0, 0, current_cmd);
        io_redir_del_redir(disk, partition.part_offset + partition.sborg_offset);
    }
    else
    {
        if (partition.upart_type == UP_NTFS || (is_part_ntfs(partition) && partition.upart_type != UP_EXFAT))
            ntfs_undelete_part(disk, partition, verbose, current_cmd);
        else
            dir_partition(disk, partition, 0, 0, current_cmd);
    }
}

static void adv_menu_list_selected(disk_t &disk, const partition_t &partition, const int verbose, const int expert,
                                   char **current_cmd)
{
    if (partition.sb_offset != 0 && partition.sb_size > 0)
    {
        io_redir_add_redir(disk, partition.part_offset + partition.sborg_offset, partition.sb_size,
                           partition.part_offset + partition.sb_offset, nullptr);
        dir_partition(disk, partition, verbose, expert, current_cmd);
        io_redir_del_redir(disk, partition.part_offset + partition.sborg_offset);
    }
    else
        dir_partition(disk, partition, verbose, expert, current_cmd);
}

static void adv_menu_superblock_selected(disk_t &disk, partition_t &partition, const int verbose, const int dump_ind,
                                         char **current_cmd)
{
    if (is_linux(partition))
    {
        list_part_t list_sb = search_superblock(disk, partition, verbose, dump_ind);
        interface_superblock(disk, list_sb, current_cmd);
    }
    if (is_hfs(partition) || is_hfsp(partition))
    {
        HFS_HFSP_boot_sector(disk, partition, verbose, current_cmd);
    }
}
