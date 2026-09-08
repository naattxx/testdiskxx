/*

    File: dimage.h

    Copyright (C) 2007 Christophe GRENIER <grenier@cgsecurity.org>

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
#ifndef _UI_DIMAGE_H
#define _UI_DIMAGE_H
#include "src/common.hpp"
#include <filesystem>

/*@
  @ requires \valid(disk_car);
  @ requires valid_disk(disk_car);
  @ requires \valid_read(partition);
  @ requires valid_read_string(image_dd);
  @ requires \separated(disk_car, partition, image_dd);
  @*/
void disk_image_interface(disk_t &disk_car, const partition_t &partition,
                          const std::filesystem::path &image_dd);

#endif
