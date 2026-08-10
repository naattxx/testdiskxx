/*

    File: hdwin32.c

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
#include <config.h>

#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(_WIN32)
#include <stdio.h>
//#include "types.h"
#include "common.hpp"
#include <stdlib.h>     /* free */
#if __has_include(<windef.h>)
#include <windef.h>
#endif
#if __has_include(<winbase.h>)
#include <stdarg.h>
#include <winbase.h>
#endif
#include <ctype.h>	/* isspace */
#if __has_include(<winioctl.h>)
#include <winioctl.h>
#endif
#if __has_include(<w32api/winioctl.h>)
#include <w32api/winioctl.h>
#endif
#include "log.hpp"
#include "hdwin32.hpp"

void file_win32_disk_get_model(HANDLE handle, disk_t &dev, const int verbose)
{
  DWORD               cbBytesReturned = 0;
  STORAGE_PROPERTY_QUERY query;
  char buffer [10240];
  memset((void *) & query, 0, sizeof (query));
  query.PropertyId = StorageDeviceProperty;
  query.QueryType = PropertyStandardQuery;
  memset(&buffer, 0, sizeof (buffer));

  if ( DeviceIoControl(handle, IOCTL_STORAGE_QUERY_PROPERTY,
	&query,
	sizeof (query),
	&buffer,
	sizeof (buffer)-1,
	&cbBytesReturned, NULL) )
  {
    const STORAGE_DEVICE_DESCRIPTOR * descrip = (const STORAGE_DEVICE_DESCRIPTOR *) & buffer;
    const unsigned int offsetVendor=descrip->VendorIdOffset;
    const unsigned int offsetProduct=descrip->ProductIdOffset;
    unsigned int lenVendor=0;
    unsigned int lenProduct=0;
    if(verbose>1)
    {
      log_info("IOCTL_STORAGE_QUERY_PROPERTY:\n");
      //dump_log(&buffer, cbBytesReturned);
    }
    buffer[cbBytesReturned]='\0';
    if(descrip->SerialNumberOffset!=0 && descrip->SerialNumberOffset < cbBytesReturned)
      dev.serial_no=strip_dup(&buffer[descrip->SerialNumberOffset]);
    if(descrip->ProductRevisionOffset!=0 && descrip->ProductRevisionOffset < cbBytesReturned)
      dev.fw_rev=strip_dup(&buffer[descrip->ProductRevisionOffset]);
    if(offsetVendor > 0 && offsetVendor < cbBytesReturned)
      lenVendor=strlen(&buffer[offsetVendor]);
    if(offsetProduct > 0 && offsetProduct < cbBytesReturned)
      lenProduct=strlen(&buffer[offsetProduct]);
    if(lenVendor+lenProduct > 0)
    {
      dev.model.reserve(lenVendor+1+lenProduct+1);
      dev.model[0]='\0';
      if(lenVendor>0 && offsetVendor + lenVendor <= cbBytesReturned)
      {
	int i;
	memcpy(dev.model.data(), &buffer[offsetVendor], lenVendor);
	dev.model[lenVendor]='\0';
	for(i=lenVendor-1;i>=0 && dev.model[i]==' ';i--);
	if(i>=0)
	  dev.model[++i]=' ';
	dev.model[++i]='\0';
      }
      if(lenProduct>0 && offsetProduct + lenProduct <= cbBytesReturned)
      {
	int i;
	strncat(dev.model.data(), &buffer[offsetProduct], lenProduct);
	for(i=dev.model.length()-1; i>=0 && dev.model[i]==' '; i--);
	dev.model[++i]='\0';
      }
      if(dev.model.length()>0)
	return ;
      dev.model.clear();
    }
  }
}
#endif
