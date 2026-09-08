/*

    File: askloc.h

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

#ifndef _ASKLOC_H
#define _ASKLOC_H

#include <filesystem>
#include <string_view>

/*@
  @ requires \valid(dst + (0 .. dst_size-1));
  @ requires valid_read_string(msg);
  @ requires \separated(dst, msg, src_dir);
  @ assigns  *dst;
  @*/
void ask_location(std::filesystem::path &dst, std::string_view msg, std::string_view src_dir);

#endif
