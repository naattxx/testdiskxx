/*

    File: autoset.c

    Copyright (C) 2009 Christophe GRENIER <grenier@cgsecurity.org>

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
#include "common.hpp"

#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_GPT)
extern const arch_fnct_t arch_gpt;
#endif
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_HUMAX)
extern const arch_fnct_t arch_humax;
#endif
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_MAC)
extern const arch_fnct_t arch_mac;
#endif

void disk_t::autoset_unit()
{
    if (
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_GPT)
        arch == &arch_gpt ||
#endif
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_HUMAX)
        arch == &arch_humax ||
#endif
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_MAC)
        arch == &arch_mac ||
#endif
        (geom.heads_per_cylinder == 1 && geom.sectors_per_head == 1))
        unit = UNIT::SECTOR;
    else
        unit = UNIT::CHS;
}
