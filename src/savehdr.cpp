/*

    File: savehdr.c

    Copyright (C) 2004-2007 Christophe GRENIER <grenier@cgsecurity.org>

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
#include <chrono>
#include <cstdint>
#include <cstring>
#include <exception>
#include <fstream>
#include <ios>
#include <print>
#include <utility>
#if __has_include(<sys/time.h>)
#include <sys/time.h>
#endif
#include "common.hpp"
#include "fnctdsk.hpp" /* get_LBA_part */
#include "log.hpp"
#include "savehdr.hpp"

auto save_header(disk_t &disk_car, const partition_t &partition, const int verbose) -> int
{
    if (verbose > 1)
    {
        log_trace("save_header");
    }
    std::ofstream f_backup = std::ofstream("header.log", std::ios::app | std::ios::binary);
    if (!f_backup.is_open())
    {
        log_critical("Can't create header.log file: {}", strerror(errno));
        return -1;
    }
    try {
      f_backup.exceptions(std::ios::badbit);
      println(f_backup, "{} {}\n{:2} {} Sys={:02X} {:5} {:3} {:2} {:5} {:3} {:2} {:10}",
              std::chrono::system_clock::now(), disk_car.description(disk_car), partition.order, std::to_underlying(partition.status),
              (disk_car.arch->get_part_type != nullptr ? disk_car.arch->get_part_type(partition) : 0),
              offset2cylinder(disk_car, partition.part_offset), offset2head(disk_car, partition.part_offset),
              offset2sector(disk_car, partition.part_offset),
              offset2cylinder(disk_car, partition.part_offset + partition.part_size - disk_car.sector_size),
              offset2head(disk_car, partition.part_offset + partition.part_size - disk_car.sector_size),
              offset2sector(disk_car, partition.part_offset + partition.part_size - disk_car.sector_size),
              partition.part_size / disk_car.sector_size);
    } catch (std::exception &e) {
      log_critical("Error while writing header.log: {}", e.what());
      return -1;
    }
    char buffer[256 * DEFAULT_SECTOR_SIZE] {};
    if (disk_car.pread(disk_car, buffer, 256 * DEFAULT_SECTOR_SIZE, partition.part_offset) !=
                        256 * DEFAULT_SECTOR_SIZE)
      return -1;
    try
    {
      f_backup.write(buffer, 256 * DEFAULT_SECTOR_SIZE);
    }
    catch (std::ios::failure &e)
    {
      log_critical("Error while writing header.log: {}", e.what());
      return -1;
    }
    return 0;
}

auto partition_load(const disk_t &disk_car, const int verbose) -> backup_disk_list_t
{
    backup_disk_t *new_backup = nullptr;
    backup_disk_list_t list_backup;

    if (verbose > 1)
    {
        log_trace("partition_load");
    }
    std::ifstream f_backup("backup.log");
    if (!f_backup.is_open())
    {
        log_error("Can't open backup.log file: {}", strerror(errno));
        return list_backup;
    }

    while (!f_backup.eof())
    {
        if (f_backup.peek() == '[')
        {
          if (new_backup != nullptr)
              list_backup.push_front(new_backup);

          new_backup = new backup_disk_t;
          f_backup.ignore(); // skip '['
          f_backup >> new_backup->my_time;

          f_backup.ignore(2);// "] "
          f_backup.getline(new_backup->description, sizeof new_backup->description);

          if (verbose > 1)
          {
              // log_verbose("new disk: [{}] {}", new_backup->my_time, new_backup->description);
          }
        }
        else if (new_backup != nullptr)
        {
          partition_t new_partition(disk_car.arch);
          char status;
          unsigned int part_type;
          uint64_t part_size;
          uint64_t part_offset;
          f_backup >> new_partition.order;
          f_backup.ignore(11); // skip ': start ='
          f_backup >> part_offset;
          f_backup.ignore(9); // skip ', size ='
          f_backup >> part_size;
          f_backup.ignore(7); // skip ', Id ='
          f_backup >> part_type;
          f_backup.ignore(2); // skip ', '
          f_backup >> status;
          if (f_backup.fail())
          {
              log_critical("partition_load: failed");
              break;
          }
          if (verbose > 1)
          {
              // log_verbose("new partition\n");
          }
          new_partition.part_offset = part_offset * disk_car.sector_size;
          new_partition.part_size = part_size * disk_car.sector_size;
          if (disk_car.arch->set_part_type != nullptr)
              disk_car.arch->set_part_type(new_partition, part_type);
          switch (status)
          {
          case 'P':
              new_partition.status = STATUS_PRIM;
              break;
          case '*':
              new_partition.status = STATUS_PRIM_BOOT;
              break;
          case 'L':
              new_partition.status = STATUS_LOG;
              break;
          default:
              new_partition.status = STATUS_DELETED;
              break;
          }
          {
              int _insert_error = 0;
              insert_new_partition(new_backup->list_part, new_partition, 0, &_insert_error);
          }
        }
        else {
          log_critical("partition_load: unexpected character: {}", f_backup.peek());
          break;
        }
    }
    if (new_backup != nullptr)
        list_backup.push_front(new_backup);
    return list_backup;
}

auto partition_save(disk_t &disk_car, const list_part_t &list_part,
                    const int verbose) -> int
{
  if (verbose > 0)
  {
    log_trace("partition_save");
  }
  std::ofstream f_backup("backup.log", std::ios::app);
  if (!f_backup.is_open())
  {
    log_critical("Can't create backup.log file: {}\n", strerror(errno));
    return -1;
  }
  std::println(f_backup, "[{}] {}",
               std::chrono::system_clock::now().time_since_epoch().count(),
               disk_car.description(disk_car));
  for (const partition_t &partition : list_part)
  {
    std::println(f_backup, "{:2} : start = {:9}, size = {:10}, Id = {:02X}, {}",
                 (partition.order < 100 ? partition.order : 0),
                 static_cast<unsigned long>(partition.part_offset /
                                            disk_car.sector_size),
                 static_cast<unsigned long>(partition.part_size /
                                            disk_car.sector_size),
                 (disk_car.arch->get_part_type != nullptr
                      ? disk_car.arch->get_part_type(partition)
                      : 0),
                 static_cast<char>(partition.status));
  }
  return 0;
}
