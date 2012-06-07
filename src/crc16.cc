/*GPL*START*
 *
 * crc engine for use with audio mpeg streams
 * 
 * Copyright (C) 1998-2001 by Johannes Overmann <Johannes.Overmann@gmx.de>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * *GPL*END*/  

#include "crc16.h"

// create crc engine and reset to 0 (build shift table)
CRC16::CRC16(unsigned int polynom): c(0)
{
   for(unsigned int i = 0; i < 256; ++i)
   {
      unsigned int x = i << 9;
      for(int j=0; j < 8; ++j, x <<= 1)
	if(x & 0x10000)
	  x ^= polynom;
      tab[i] = x>>1;
   }
}
