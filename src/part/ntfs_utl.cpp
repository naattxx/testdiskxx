/**
 * ntfs_utl.c - Part of the TestDisk project.
 *
 * Copyright (c) 2004-2007 Christophe Grenier
 *
 * Original version comes from the Linux-NTFS project.
 * Copyright (c) 2003 Lode Leroy
 * Copyright (c) 2003 Anton Altaparmakov
 * Copyright (c) 2003 Richard Russon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (in the main directory of the Linux-NTFS
 * distribution in the file COPYING); if not, write to the Free Software
 * Foundation,Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include <config.h>

#if defined(DISABLED_FOR_FRAMAC)
#undef HAVE_LIBNTFS
#undef HAVE_LIBNTFS3G
#endif

#if defined(HAVE_LIBNTFS) || defined(HAVE_LIBNTFS3G)
#include <cstdio>
#include <cstdlib>
#if __has_include(<sys/stat.h>)
#include <sys/stat.h>
#endif
#include <cerrno>
#include <cstring>
#include <ctime>
#include <utility>
#if __has_include(<sys/param.h>)
#include <sys/param.h>
#endif
#if __has_include(<machine/endian.h>)
#include <machine/endian.h>
#endif
#include <cstdarg>
// #include "types.h"

#ifdef HAVE_LIBNTFS
#include <ntfs/attrib.h>
#endif
#ifdef HAVE_LIBNTFS3G
#define HAVE_SYS_TYPES_H
#define HAVE_SYS_STAT_H
#define HAVE_TIME_H
#define HAVE_STDDEF_H
extern "C"
{
#include <ntfs-3g/attrib.h>
}
#undef min
#undef max
#endif

#include "ntfs_utl.hpp"
#include "src/common.hpp"
#include "src/log.hpp"

/**
 * find_attribute - Find an attribute of the given type
 * @type:  An attribute type, e.g. AT_FILE_NAME
 * @ctx:   A search context, created using ntfs_get_attr_search_ctx
 *
 * Using the search context to keep track, find the first/next occurrence of a
 * given attribute type.
 *
 * N.B.  This will return a pointer into @mft.  As long as the search context
 *       has been created without an inode, it won't overflow the buffer.
 *
 * Return:  Pointer  Success, an attribute was found
 *	    NULL     Error, no matching attributes were found
 */
auto find_attribute(const ATTR_TYPES type, ntfs_attr_search_ctx *ctx)
    -> ATTR_RECORD *
{
  if (!ctx)
  {
    errno = EINVAL;
    return nullptr;
  }

  if (ntfs_attr_lookup(type, nullptr, 0, CASE_SENSITIVE, 0, nullptr, 0, ctx) !=
      0)
  {
#ifdef DEBUG_NTFS
    log_debug("find_attribute didn't find an attribute of type: 0x%02x.\n",
              type);
#endif
    return nullptr; /* None / no more of that type */
  }
#ifdef DEBUG_NTFS
  log_debug("find_attribute found an attribute of type: 0x%02x.\n", type);
#endif
  return ctx->attr;
}

/**
 * find_first_attribute - Find the first attribute of a given type
 * @type:  An attribute type, e.g. AT_FILE_NAME
 * @mft:   A buffer containing a raw MFT record
 *
 * Search through a raw MFT record for an attribute of a given type.
 * The return value is a pointer into the MFT record that was supplied.
 *
 * N.B.  This will return a pointer into @mft.  The pointer won't stray outside
 *       the buffer, since we created the search context without an inode.
 *
 * Return:  Pointer  Success, an attribute was found
 *	    NULL     Error, no matching attributes were found
 */
auto find_first_attribute(const ATTR_TYPES type, MFT_RECORD *mft)
    -> ATTR_RECORD *
{
  ntfs_attr_search_ctx *ctx;
  ATTR_RECORD *rec;

  if (!mft)
  {
    errno = EINVAL;
    return nullptr;
  }

  ctx = ntfs_attr_get_search_ctx(nullptr, mft);
  if (!ctx)
  {
    log_error("Couldn't create a search context.\n");
    return nullptr;
  }

  rec = find_attribute(type, ctx);
  ntfs_attr_put_search_ctx(ctx);
#ifdef DEBUG_NTFS
  if (rec)
    log_debug("find_first_attribute: found attr of type 0x%02x.\n", type);
  else
    log_debug("find_first_attribute: didn't find attr of type 0x%02x.\n", type);
#endif
  return rec;
}

/**
 * utils_cluster_in_use - Determine if a cluster is in use
 * @vol:  An ntfs volume obtained from ntfs_mount
 * @lcn:  The Logical Cluster Number to test
 *
 * The metadata file $Bitmap has one binary bit representing each cluster on
 * disk.  The bit will be set for each cluster that is in use.  The function
 * reads the relevant part of $Bitmap into a buffer and tests the bit.
 *
 * This function has a static buffer in which it caches a section of $Bitmap.
 * If the lcn, being tested, lies outside the range, the buffer will be
 * refreshed.
 *
 * Return:  1  Cluster is in use
 *	    0  Cluster is free space
 *	   -1  Error occurred
 */
auto utils_cluster_in_use(ntfs_volume *vol, long long lcn) -> int
{
  static unsigned char buffer[512];
  static long long bmplcn =
      -sizeof(buffer) - 1; /* Which bit of $Bitmap is in the buffer */

  int byte, bit;

  if (!vol)
  {
    errno = EINVAL;
    return -1;
  }

  /* Does lcn lie in the section of $Bitmap we already have cached? */
  if ((bmplcn < 0) || (std::cmp_less(lcn, bmplcn)) ||
      (std::cmp_greater_equal(lcn,
                              (static_cast<unsigned>(bmplcn) +
                               (static_cast<unsigned>(sizeof(buffer)) << 3)))))
  {
    ntfs_attr *attr;
#ifdef DEBUG_NTFS
    log_debug("Bit lies outside cache.\n");
#endif
    attr = ntfs_attr_open(vol->lcnbmp_ni, AT_DATA, AT_UNNAMED, 0);
    if (!attr)
    {
      log_error("Couldn't open $Bitmap\n");
      return -1;
    }

    /* Mark the buffer as in use, in case the read is shorter. */
    memset(buffer, 0xFF, sizeof(buffer));
    bmplcn = lcn & (~((sizeof(buffer) << 3) - 1));

    if (ntfs_attr_pread(attr, (bmplcn >> 3), sizeof(buffer), buffer) < 0)
    {
      log_error("Couldn't read $Bitmap\n");
      ntfs_attr_close(attr);
      return -1;
    }
#ifdef DEBUG_NTFS
    log_debug("Reloaded bitmap buffer.\n");
#endif
    ntfs_attr_close(attr);
  }

  bit  = 1 << (lcn & 7);
  byte = (lcn >> 3) & (sizeof(buffer) - 1);
#ifdef DEBUG_NTFS
  log_debug("cluster = %lld, bmplcn = %lld, byte = %d, bit = %d, in use %d\n",
            lcn, bmplcn, byte, bit, buffer[byte] & bit);
#endif
  return (buffer[byte] & bit);
}

#endif
