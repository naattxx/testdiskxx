/*

    File: ext2_sb.c

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
#include <config.h>

#include <cstdio>
#include <cstring>
// #include "types.h"
#include "src/common.hpp"
#include "src/intrf.hpp"
#include "src/lang.h"
#ifdef HAVE_NCURSES
#include "intrfn.h"
#endif
#include "ext2_sb.hpp"
#include "src/guid_cmp.hpp"
#include "src/log.hpp"

auto interface_superblock(disk_t &disk_car, const list_part_t &list_part, char **current_cmd) -> int
{
    const partition_t *old_part = nullptr;
#ifdef HAVE_NCURSES
    const struct MenuItem menuSuperblock[] = {
        {'P', "Previous", ""}, {'N', "Next", ""}, {'Q', "Quit", "Return to Advanced menu"}, {0, NULL, NULL}};
#endif
    screen_buffer_reset();
#ifdef HAVE_NCURSES
    aff_copy(stdscr);
    wmove(stdscr, 4, 0);
    wprintw(stdscr, "%s", disk_car->description(disk_car));
    wmove(stdscr, 5, 0);
    mvwaddstr(stdscr, 6, 0, msg_PART_HEADER_LONG);
#endif
    for (const partition_t &partition : list_part)
    {
        if (old_part == nullptr || old_part->part_offset != partition.part_offset ||
            old_part->part_size != partition.part_size ||
            guid_cmp(old_part->part_type_gpt, partition.part_type_gpt) != 0 ||
            old_part->part_type_i386 != partition.part_type_i386 ||
            old_part->part_type_sun != partition.part_type_sun || old_part->part_type_mac != partition.part_type_mac ||
            old_part->upart_type != partition.upart_type)
        {
            aff_part_buffer(AFF_PART_BASE, disk_car, partition);
            old_part = &partition;
        }
        if (partition.blocksize != 0)
            screen_buffer_add("superblock {}, blocksize={} [%s]\n",
                              static_cast<long unsigned>(partition.sb_offset / partition.blocksize),
                              partition.blocksize, partition.fsname);
    }
    if (!list_part.empty())
    {
        const partition_t &partition = list_part.front();
        screen_buffer_add("\n");
        screen_buffer_add("To repair the filesystem using alternate superblock, run\n");
        screen_buffer_add("fsck.ext{} -p -b superblock -B blocksize device\n",
                          (partition.upart_type == UP_EXT2 ? 2 : (partition.upart_type == UP_EXT3 ? 3 : 4)));
    }
    screen_buffer_to_log();
    if (*current_cmd == nullptr)
    {
        ; // log_flush();
#ifdef HAVE_NCURSES
        screen_buffer_display(stdscr, "", menuSuperblock);
#endif
    }
    return 0;
}
