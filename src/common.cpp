/*

    File: common.c

    Copyright (C) 1998-2006 Christophe GRENIER <grenier@cgsecurity.org>

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
#include <algorithm>
#include <chrono>
#include <config.h>
#include <cstdint>
#include <string_view>

#ifdef DISABLED_FOR_FRAMAC
#undef HAVE_POSIX_MEMALIGN
#undef HAVE_MEMALIGN
#undef HAVE_NCURSES
#endif

#include <cctype>
#include <cstdio>
#include <cstdlib>
// #include <malloc.h>
#include <cstring>
#ifdef __MINGW32__
#if __has_include(<io.h>)
#include <io.h>
#endif
#endif
#include <cassert>
#include <ctime>
// #include "types.h"
#include "common.hpp"
#include "log.hpp"

static long secwest = 0;

void partition_t::set_name(std::string_view src)
{
  fsname = src.substr(0, src.find('\0'));
}

void partition_t::set_name_chomp(std::string_view src)
{
  fsname = src.substr(0, src.find('\0'));

  fsname.erase(std::ranges::find_if(
                   fsname.rbegin(), fsname.rend(),
                   [](unsigned char ch) -> bool { return !std::isspace(ch); }
               ).base(),
               fsname.end());
}

auto strip_dup(char *str) -> char *
{
    char *end;
    char *tmp;
    /*@
      @ loop invariant valid_string(str);
      @ loop assigns str;
      @ loop variant strlen(\at(str, Pre)) - strlen(str);
      @*/
    while (isspace(*str))
        str++;
    end = str;
    /*@ assert valid_string(end); */
    /*@
      @ loop invariant valid_string(tmp);
      @ loop invariant valid_string(end);
      @ loop invariant end == str || *end != '\0';
      @ loop assigns tmp, end;
      @ loop variant strlen(str) - strlen(tmp);
      @*/
    for (tmp = str; *tmp != 0; tmp++)
        if (!isspace(*tmp))
            end = tmp;
    /*@ assert valid_string(end); */
    if (str == end)
        return nullptr;
    *(end + 1) = 0;
    return strdup(str);
}

/* Convert a MS-DOS time/date pair to a UNIX date (seconds since 1 1 70). */
/*
 * The epoch of FAT timestamp is 1980.
 *     :  bits :     value
 * date:  0 -  4: day	(1 -  31)
 * date:  5 -  8: month	(1 -  12)
 * date:  9 - 15: year	(0 - 127) from 1980
 * time:  0 -  4: sec	(0 -  29) 2sec counts
 * time:  5 - 10: min	(0 -  59)
 * time: 11 - 15: hour	(0 -  23)
 */

/*@
  @ requires -14*3600 <= secwest <= 12*3600;
  @ terminates \true;
  @ assigns \nothing;
  @*/
auto date_dos2unix(const uint16_t f_time, const uint16_t f_date) -> time_t
{
  short year  = (f_date >> 9) + 1980;
  short month = std::max(1, (f_date >> 5) & 0xf);
  short day   = std::max(1, f_date & 0x1f);
  std::chrono::year_month_day ymd{
      std::chrono::year{year},
      std::chrono::month{static_cast<unsigned>(month)},
      std::chrono::day{
                        static_cast<unsigned>(day),
                        }
  };

  short hour   = (f_time >> 11) & 0x1F;
  short minute = (f_time >> 5) & 0x3F;
  short second = (f_time & 0x1F) * 2;

  auto tp = std::chrono::sys_days{ymd} + std::chrono::hours{hour} +
            std::chrono::minutes{minute} + std::chrono::seconds{second};

#ifdef __FRAMAC__
  return std::chrono::system_clock::to_time_t(tp);
#else
  return std::chrono::system_clock::to_time_t(tp) + secwest;
#endif
}

void set_secwest()
{
    const time_t t = time(nullptr);
#if defined(__MINGW32__) || defined(DISABLED_FOR_FRAMAC)
    const struct tm *tmptr = localtime(&t);
#else
    struct tm tmp;
    const struct tm *tmptr = localtime_r(&t, &tmp);
#endif
#ifdef HAVE_STRUCT_TM_TM_GMTOFF
    if (tmptr)
        secwest = -1 * tmptr->tm_gmtoff;
    else
        secwest = 0;
#elif defined(DJGPP) || defined(__ANDROID__)
    secwest = 0;
#else
#ifdef __CYGWIN__
    secwest = _timezone;
#else
    secwest = timezone;
#endif
#ifdef __FRAMAC__
    if (secwest < -48 * 3600)
    {
        secwest = 0;
        return;
    }
#endif
    /* account for daylight savings */
    if (tmptr && tmptr->tm_isdst)
        secwest -= 3600;
#endif
}

/**
 * td_ntfs2utc - Convert an NTFS time to Unix time
 * @time:  An NTFS time in 100ns units since 1601
 *
 * NTFS stores times as the number of 100ns intervals since January 1st 1601 at
 * 00:00 UTC.  This system will not suffer from Y2K problems until ~57000AD.
 *
 * Return:  n  A Unix time (number of seconds since 1970)
 */
#define NTFS_TIME_OFFSET ((int64_t)(369 * 365 + 89) * 24 * 3600 * 10000000)
auto td_ntfs2utc(int64_t ntfstime) -> time_t
{
    if (ntfstime < NTFS_TIME_OFFSET)
        return 0;
    return (ntfstime - NTFS_TIME_OFFSET) / 10000000;
}

auto check_command(char **current_cmd, const char *cmd, const size_t n) -> int
{
    const int res = strncmp(*current_cmd, cmd, n);
    if (res == 0)
    {
        (*current_cmd) += n;
        /*@ assert valid_read_string(*current_cmd); */
        return 0;
    }
    /*@ assert valid_read_string(*current_cmd); */
    return res;
}

void skip_comma_in_command(char **current_cmd)
{
    /*@
      @ loop invariant valid_read_string(*current_cmd);
      @ loop assigns *current_cmd;
      @ loop variant strlen(*current_cmd);
      */
    while (*current_cmd[0] == ',')
    {
        (*current_cmd)++;
    }
    /*@ assert valid_read_string(*current_cmd); */
}

auto get_int_from_command(char **current_cmd) -> uint64_t
{
    uint64_t tmp = 0;
    /*@
      @ loop invariant valid_read_string(*current_cmd);
      @ loop assigns *current_cmd, tmp;
      @ loop variant strlen(*current_cmd);
      @*/
    while (*current_cmd[0] >= '0' && *current_cmd[0] <= '9')
    {
#ifdef __FRAMAC__
        const unsigned int v = *current_cmd[0] - '0';
        /*@ assert 0 <= v <= 9; */
        if (tmp >= UINT64_MAX / 10)
            return tmp;
        /** assert tmp < UINT64_MAX / 10; */
        tmp *= 10;
        /** assert tmp <= UINT64_MAX - 10; */
        tmp += v;
#else
        tmp = tmp * 10 + (*current_cmd[0] - '0');
#endif
        (*current_cmd)++;
    }
    return tmp;
}
