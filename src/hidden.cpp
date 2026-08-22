/*

    File: hidden.c

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

#include "common.hpp"
#include "log.hpp"

auto disk_t::is_hpa_or_dco() const -> int
{
    int res = 0;
    if (native_max > 0 && user_max < native_max + 1)
    {
        res = 1;
        if (native_max < dco)
            res |= 2;
    }
    else if (dco > 0 && user_max < dco + 1)
    {
#ifndef DISABLED_FOR_FRAMAC
        log_info("user_max={} dco={}\n", user_max, dco);
#endif
        res |= 2;
    }
#ifndef DISABLED_FOR_FRAMAC
    if (res > 0)
    {
        if (res & 1)
            log_warning("{}: Host Protected Area (HPA) present.\n", device);
        if (res & 2)
            log_warning("{}: Device Configuration Overlay (DCO) present.\n", device);
        // log_flush();
    }
#endif
    return res;
}
