/*

    File: ewf.h

    Copyright (C) 2006 Christophe GRENIER <grenier@cgsecurity.org>

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
#ifndef _EWF_H
#define _EWF_H

#include <config.h>

#ifdef DISABLED_FOR_FRAMAC
#undef HAVE_LIBEWF
#endif

#if __has_include(<libewf.h>) && defined(HAVE_LIBEWF)
#include "src/common.hpp"
#include <optional>
/*@
  @ requires valid_read_string(device);
  @ ensures  valid_disk(\result);
  @*/
auto fewf_init(const char *device, const int testdisk_mode) -> std::optional<disk_t>;
#endif
/*@ assigns \nothing; */
auto td_ewf_version() -> const char*;

#endif
