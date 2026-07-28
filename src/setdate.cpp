/*

    File: setdate.c

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

#if defined(DISABLED_FOR_FRAMAC)
#undef HAVE_UTIME
#endif

// #include "types.h"
#if __has_include("utime.h")
#include <utime.h>
#endif
#include "log.hpp"
#include "setdate.hpp"
#include <stdio.h>

/**
 * set_date - Set the file's date and time
 * @pathname:  Path and name of the file to alter
 * @actime:    Date and time to set
 * @modtime:   Date and time to set
 *
 * Give a file a particular date and time.
 *
 * Return:  0  Success, set the file's date and time
 *	    -1  Error, failed to change the file's date and time
 */
int set_date(const char *pathname, time_t actime, time_t modtime)
{
#if __has_include("utime.h")
    struct utimbuf ut;
    if (!pathname)
        return -1;
    ut.actime = actime;
    ut.modtime = modtime;
    if (utime(pathname, &ut))
    {
        log_error("ERROR: Couldn't set the file's date and time for %s\n", pathname);
        return -1;
    }
#endif
    return 0;
}
