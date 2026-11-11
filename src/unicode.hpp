/*

    File: unicode.h

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
#ifndef _UNICODE_H
#define _UNICODE_H
#include <cstddef>
#include <simdutf.h>
#include <span>
#include <string>
#include <string_view>

/*@
  @ requires \valid(to + ( 0 .. len-1));
  @ requires \valid_read(from + ( 0 .. len-1));
  @ requires \separated(to + (..), from + (..));
  @ terminates \true;
  @ assigns to[0 .. len-1];
  @*/
[[nodiscard]]
constexpr auto UCSle2str(std::string &to,
                         std::span<const char16_t> from) noexcept -> size_t
{
  to.resize(simdutf::utf8_length_from_utf16le(from));
  return simdutf::convert_utf16le_to_utf8(from, to);
}

/*@
  @ requires \valid(to + ( 0 .. len-1));
  @ requires \valid_read(from + ( 0 .. len-1));
  @ requires \separated(to + (..), from + (..));
  @ terminates \true;
  @ assigns to[0 .. len-1];
  @*/
[[nodiscard]]
constexpr auto str2UCSle(std::span<char16_t> to, std::string_view from) noexcept
    -> size_t
{
  return simdutf::convert_utf8_to_utf16le(from, to);
}

#endif
