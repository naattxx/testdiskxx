/*

    File: unicode.c

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

#include <config.h>
#include <span>
// #include "types.h"
#include "common.hpp"
#include "unicode.hpp"

auto UCSle2str(std::string &to, std::span<const uint16_t> from) -> unsigned int
{
    unsigned int i;
    to.resize(from.size());
    /*@
      @ loop assigns i, to[0 .. i];
      @ loop variant len - i;
      @*/
    for (i = 0; i < from.size() && le16(from[i]) != 0; i++)
    {
        if (le16(from[i]) & 0xff00)
            to[i] = '?';
        else
            to[i] = static_cast<char>(le16(from[i]));
    }
    if (i < from.size())
        to[i] = '\0';
    return i;
}

auto str2UCSle(uint16_t *to, std::string_view from, const unsigned int len) -> unsigned int
{
    unsigned int i;
    /*@
      @ loop assigns i, to[0 .. i];
      @ loop variant len - i;
      @*/
    for (i = 0; (i < len) && from[i]; i++)
    {
        to[i] = le16(from[i]);
    }
    if (i < len)
        to[i] = '\0';
    return i;
}
