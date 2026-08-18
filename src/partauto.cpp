/*

    File: partauto.c

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

#include <cstdio>
#include <cstring>
// #include "types.h"
#include "common.hpp"
#include "fnctdsk.hpp"
#include "log.hpp"

extern const arch_fnct_t arch_none;
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_GPT)
extern const arch_fnct_t arch_gpt;
#endif
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_HUMAX)
extern const arch_fnct_t arch_humax;
#endif
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_I386)
extern const arch_fnct_t arch_i386;
#endif
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_MAC)
extern const arch_fnct_t arch_mac;
#endif
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_SUN)
extern const arch_fnct_t arch_sun;
#endif
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_XBOX)
extern const arch_fnct_t arch_xbox;
#endif

void disk_t::autodetect_arch(const arch_fnct_t *default_arch)
{
    list_part_t list_part;
#ifdef DEBUG_PARTAUTO
    const int verbose = 2;
#else
    const int verbose = 0;
    // unsigned int old_levels;
    // old_levels=log_set_levels(0);
#endif
    {
        arch = &arch_none;
        list_part = arch_none.read_part(*this, verbose, 0);
        /*@ assert valid_list_part(list_part); */
        if (!list_part.empty() && list_part.front().upart_type == UP_UNK)
        {
            list_part.clear();
        }
    }
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_XBOX)
    if (list_part.empty())
    {
        arch = &arch_xbox;
        list_part = arch_xbox.read_part(*this, verbose, 0);
        /*@ assert valid_list_part(list_part); */
    }
#endif
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_GPT)
    if (list_part.empty())
    {
        arch = &arch_gpt;
        list_part = arch_gpt.read_part(*this, verbose, 0);
        /*@ assert valid_list_part(list_part); */
    }
#endif
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_HUMAX)
    if (list_part.empty())
    {
        arch = &arch_humax;
        list_part = arch_humax.read_part(*this, verbose, 0);
        /*@ assert valid_list_part(list_part); */
    }
#endif
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_I386)
    if (list_part.empty())
    {
        arch = &arch_i386;
        list_part = arch_i386.read_part(*this, verbose, 0);
        /*@ assert valid_list_part(list_part); */
    }
#endif
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_SUN)
    if (list_part.empty())
    {
        arch = &arch_sun;
        list_part = arch_sun.read_part(*this, verbose, 0);
        /*@ assert valid_list_part(list_part); */
    }
#endif
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_MAC)
    if (list_part.empty())
    {
        arch = &arch_mac;
        list_part = arch_mac.read_part(*this, verbose, 0);
        /*@ assert valid_list_part(list_part); */
    }
#endif
#ifndef DEBUG_PARTAUTO
    // log_set_levels(old_levels);
#endif
    if (!list_part.empty())
    {
        arch_autodetected = arch;
        log_info("Partition table type (auto): {}\n", arch->part_name);
        return;
    }
    arch_autodetected = nullptr;
    if (default_arch != nullptr)
    {
        arch = default_arch;
    }
    else
    {
#ifdef DISABLED_FOR_FRAMAC
        arch = &arch_none;
#elif defined(TARGET_SOLARIS) && (!defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_SUN))
        arch = &arch_sun;
#elif defined(__APPLE__) && defined(TESTDISK_LSB) && (!defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_GPT))
        arch = &arch_gpt;
#elif defined(__APPLE__) && !defined(TESTDISK_LSB) && (!defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_MAC))
        arch = &arch_mac;
#else
#if defined(__CYGWIN__) || defined(__MINGW32__)
        if (device[0] == '\\' && device[1] == '\\' && device[2] == '.' && device[3] == '\\' &&
            device[5] == ':')
            arch = &arch_none;
        else
#endif
#if !defined(SINGLE_PARTITION_TYPE) || (defined(SINGLE_PARTITION_I386) && defined(SINGLE_PARTITION_GPT))
            /* PC/Intel partition table is limited to 2 TB, 2^32 512-bytes sectors */
            if (disk_size < (static_cast<uint64_t>(1) << (32 + 9)))
                arch = &arch_i386;
            else
                arch = &arch_gpt;
#else
        arch = &arch_none;
#endif
#endif
    }
    log_info("Partition table type defaults to {}\n", arch->part_name);
}
