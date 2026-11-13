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
#include "adv.hpp"
#include "common.hpp"

extern const arch_fnct_t arch_gpt;
extern const arch_fnct_t arch_i386;
extern const arch_fnct_t arch_mac;
extern const arch_fnct_t arch_none;
extern const arch_fnct_t arch_sun;
extern const arch_fnct_t arch_xbox;

auto is_part_linux(const partition_t &partition) noexcept -> int
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
