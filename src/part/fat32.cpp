/*

    File: fat32.c

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

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
// #include "types.h"
#include "src/common.hpp"
#include "src/intrf.hpp"
#include "src/lang.h"
// #include "src/intrfn.hpp"
#include "fat.hpp"
#include "fat32.hpp"
#include "fat_adv.hpp"
#include "src/dirpart.hpp"
#include "src/io_redir.hpp"
#include "src/log.hpp"
#include "src/log_part.hpp"

#ifdef HAVE_NCURSES
static void dump_fat32_ncurses(disk_t &disk_car, const partition_t &partition,
                               const unsigned char *buffer_bs,
                               const unsigned char *buffer_backup_bs)
{
  WINDOW *window = newwin(LINES, COLS, 0, 0); /* full screen */
  keypad(window, TRUE);                       /* Need it to get arrow key */
  aff_copy(window);
  wmove(window, 4, 0);
  wprintw(window, "%s", disk_car->description(disk_car));
  wmove(window, 5, 0);
  aff_part(window, AFF_PART_ORDER | AFF_PART_STATUS, disk_car, partition);
  mvwaddstr(window, 6, 0,
            "Boot sector                        Backup boot sector");
  dump2(window, buffer_bs, buffer_backup_bs, 3 * disk_car->sector_size);
  delwin(window);
  (void)clearok(stdscr, TRUE);
#ifdef HAVE_TOUCHWIN
  touchwin(stdscr);
#endif
}
#endif

static void dump_fat32(disk_t &disk_car, const partition_t &partition,
                       const unsigned char *buffer_bs,
                       const unsigned char *buffer_backup_bs,
                       char **current_cmd)
{
  // log_info("Boot sector                        Backup boot sector\n");
  // dump2_log(buffer_bs, buffer_backup_bs, 3*disk_car.sector_size);
  log_fat2_info(
      reinterpret_cast<const struct fat_boot_sector *>(buffer_bs),
      reinterpret_cast<const struct fat_boot_sector *>(buffer_backup_bs),
      UP_FAT32, disk_car.sector_size
  );
  if (*current_cmd == nullptr)
  {
#ifdef HAVE_NCURSES
    dump_fat32_ncurses(disk_car, partition, buffer_bs, buffer_backup_bs);
#endif
  }
}

auto fat32_boot_sector(disk_t &disk_car, partition_t &partition,
                       const int verbose, const int dump_ind,
                       const unsigned int expert, char **current_cmd) -> int
{
  unsigned char *buffer_bs;
  unsigned char *buffer_backup_bs;
#ifdef HAVE_NCURSES
  const struct MenuItem menu_fat32[] = {
      {'P', "Previous",   ""                                        },
      {'N', "Next",       ""                                        },
      {'Q', "Quit",       "Return to Advanced menu"                 },
      {'L', "List",
       "List directories and files, copy and undelete data from FAT"},
      {'O', "Org. BS",    "Copy boot sector over backup sector"     },
      {'B', "Backup BS",  "Copy backup boot sector over boot sector"},
      {'R', "Rebuild BS", "Rebuild boot sector"                     },
      {'D', "Dump",       "Dump boot sector and backup boot sector" },
      {'C', "Repair FAT", "Very Dangerous! Expert only"             },
      {0,   NULL,         NULL                                      }
  };
#endif
  buffer_bs        = new unsigned char[3 * disk_car.sector_size];
  buffer_backup_bs = new unsigned char[3 * disk_car.sector_size];
  while (true)
  {
    const char *options;
    unsigned int menu = 0;
    int command;
    screen_buffer_reset();
    {
      int opt_over = 0;
      int opt_B    = 0;
      int opt_O    = 0;
#ifdef HAVE_NCURSES
      aff_copy(stdscr);
      wmove(stdscr, 4, 0);
      wprintw(stdscr, "%s", disk_car->description(disk_car));
      mvwaddstr(stdscr, 5, 0, msg_PART_HEADER_LONG);
      wmove(stdscr, 6, 0);
      aff_part(stdscr, AFF_PART_ORDER | AFF_PART_STATUS, disk_car, partition);
#endif
      log_info("\nfat32_boot_sector\n");
      log_partition(disk_car, partition);
      screen_buffer_add("Boot sector\n");
      if (static_cast<unsigned>(disk_car.pread(disk_car, buffer_bs,
                                               3 * disk_car.sector_size,
                                               partition.part_offset)) !=
          3 * disk_car.sector_size)
      {
        screen_buffer_add("fat32_boot_sector: Can't read boot sector.\n");
        memset(buffer_bs, 0, 3 * disk_car.sector_size);
      }
      if (test_FAT(disk_car,
                   reinterpret_cast<struct fat_boot_sector *>(buffer_bs),
                   partition, verbose, 0) == 0)
      {
        screen_buffer_add("OK\n");
        if (partition.upart_type == UP_FAT32)
        {
          opt_O    = 1;
          opt_over = 1;
        }
        else
        {
          screen_buffer_add(
              "Warning: valid FAT bootsector but not a FAT32 one!"
          );
        }
      }
      else
      {
        screen_buffer_add("Bad\n");
      }
      screen_buffer_add("\nBackup boot sector\n");
      if (static_cast<unsigned>(disk_car.pread(
              disk_car, buffer_backup_bs, 3 * disk_car.sector_size,
              partition.part_offset + 6 * disk_car.sector_size
          )) != 3 * disk_car.sector_size)
      {
        screen_buffer_add(
            "fat32_boot_sector: Can't read backup boot sector.\n"
        );
        memset(buffer_backup_bs, 0, 3 * disk_car.sector_size);
      }
      if (test_FAT(disk_car,
                   reinterpret_cast<struct fat_boot_sector *>(buffer_backup_bs),
                   partition, verbose, 0) == 0)
      {
        screen_buffer_add("OK\n");
        if (partition.upart_type == UP_FAT32)
        {
          opt_B    = 1;
          opt_over = 1;
        }
        else
        {
          screen_buffer_add(
              "Warning: valid FAT backup bootsector but not a FAT32 one!"
          );
        }
      }
      else
      {
        screen_buffer_add("Bad\n");
      }
      screen_buffer_add("\n");
      if ((memcmp(buffer_bs, buffer_backup_bs, 0x3E8) == 0) &&
          (memcmp(buffer_bs + 0x3F0, buffer_backup_bs + 0x3F0,
                  0x600 - 0x3F0)) == 0)
      {
        screen_buffer_add("Sectors are identical.\n");
        opt_over = 0;
      }
      else
      {
        if (memcmp(buffer_bs, buffer_backup_bs, 0x200) != 0)
          screen_buffer_add(
              "First sectors (boot code and partition information) are not "
              "identical.\n"
          );
        if ((memcmp(buffer_bs + disk_car.sector_size,
                    buffer_backup_bs + disk_car.sector_size, 0x1E8) != 0) ||
            (memcmp(buffer_bs + disk_car.sector_size + 0x1F0,
                    buffer_backup_bs + disk_car.sector_size + 0x1F0,
                    0x200 - 0x1F0) != 0))
          screen_buffer_add(
              "Second sectors (cluster information) are not identical.\n"
          );
        if (memcmp(buffer_bs + 2 * disk_car.sector_size,
                   buffer_backup_bs + 2 * disk_car.sector_size, 0x200) != 0)
          screen_buffer_add(
              "Third sectors (second part of boot code) are not identical.\n"
          );
      }
      screen_buffer_add("\n");
      screen_buffer_add(
          "A valid FAT Boot sector must be present in order to access\n"
      );
      screen_buffer_add("any data; even if the partition is not bootable.\n");
      if (opt_over != 0)
      {
        //	assert(opt_B>0 || opt_O>0);
        if (opt_B != 0 && opt_O != 0)
          options = "DOBRL";
        else if (opt_B != 0)
        {
          partition.sb_offset = 6 * disk_car.sector_size;
          menu                = 5;
          options             = "DBRL";
        }
        else
        {
          menu    = 4;
          options = "DORL";
        }
      }
      else
      {
        if (opt_B != 0)
          options = "DRCL";
        else
          options = "DR";
      }
    }
    screen_buffer_to_log();
    if (*current_cmd != nullptr)
    {
      command = 0;
      skip_comma_in_command(current_cmd);
      if (check_command(current_cmd, "rebuildbs", 9) == 0)
      {
        command = 'R';
      }
      else if (check_command(current_cmd, "dump", 4) == 0)
      {
        command = 'D';
      }
      else if (check_command(current_cmd, "list", 4) == 0)
      {
        if (strchr(options, 'L') != nullptr)
          command = 'L';
      }
      else if (check_command(current_cmd, "repairfat", 9) == 0)
      {
        if (strchr(options, 'C') != nullptr)
          command = 'C';
      }
      else if (check_command(current_cmd, "originalfat", 11) == 0)
      {
        if (strchr(options, 'O') != nullptr)
          command = 'O';
      }
      else if (check_command(current_cmd, "backupfat", 9) == 0)
      {
        if (strchr(options, 'B') != nullptr)
          command = 'B';
      }
    }
    else
    {
      ; // log_flush();
#ifdef HAVE_NCURSES
      command = screen_buffer_display_ext(stdscr, options, menu_fat32, &menu);
#else
      command = 0;
#endif
    }
    switch (command)
    {
    case 0:
      delete[] (buffer_bs);
      delete[] (buffer_backup_bs);
      return 0;
    case 'O': /* O : copy original boot sector over backup boot */
#ifdef HAVE_NCURSES
      if (ask_confirmation("Copy original FAT32 boot sector over backup boot, "
                           "confirm ? (Y/N)") != 0)
      {
        log_info("copy original boot sector over backup boot\n");
        if ((unsigned)disk_car->pwrite(
                disk_car, buffer_bs, 3 * disk_car->sector_size,
                partition.part_offset + 6 * disk_car->sector_size
            ) != 3 * disk_car->sector_size)
        {
          display_message(
              "Write error: Can't overwrite FAT32 backup boot sector\n"
          );
        }
        disk_car->sync(disk_car);
      }
#endif
      break;
    case 'B': /* B : copy backup boot sector over boot sector */
      /* Reset information about backup boot sector */
      partition.sb_offset = 0;
#ifdef HAVE_NCURSES
      if (ask_confirmation(
              "Copy backup FAT32 boot sector over boot sector, confirm ? (Y/N)"
          ) != 0)
      {
        log_info("copy backup boot sector over boot sector\n");
        if ((unsigned)disk_car->pwrite(disk_car, buffer_backup_bs,
                                       3 * disk_car->sector_size,
                                       partition.part_offset) !=
            3 * disk_car->sector_size)
        {
          display_message("Write error: Can't overwrite FAT32 boot sector\n");
        }
        disk_car->sync(disk_car);
      }
#endif
      break;
    case 'C':
      repair_FAT_table(disk_car, partition, verbose, current_cmd);
      break;
    case 'D':
      dump_fat32(disk_car, partition, buffer_bs, buffer_backup_bs, current_cmd);
      break;
    case 'L':
      if (strchr(options, 'O') == nullptr && strchr(options, 'B') != nullptr)
      {
        io_redir_add_redir(disk_car, partition.part_offset,
                           3 * disk_car.sector_size, 0, buffer_backup_bs);
        dir_partition(disk_car, partition, 0, 0, current_cmd);
        io_redir_del_redir(disk_car, partition.part_offset);
      }
      else
        dir_partition(disk_car, partition, 0, 0, current_cmd);
      break;
    case 'R': /* R : rebuild boot sector */
      rebuild_FAT_BS(disk_car, partition, verbose, dump_ind, expert,
                     current_cmd);
      break;
    }
  }
}
