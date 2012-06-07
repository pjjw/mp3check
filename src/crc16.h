/*GPL*START*
 *
 * crc engine for use with audio mpeg streams header file
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


// 1998:
// 21 Jan  started
// 22 Jan  first usable version: modified and tested for use with audio mpeg
// 23 Jan  bit reversal in init removed
// 27 Jan  doc and init c(0) in cons

// 2000:
// 10 Jul 01:00 ttypes.h removed

// 2001:
// 26 Feb 00:00 Khali added file crc16.cc for better code structure
// 26 Feb 00:00 reset(), add() and crc() made inline again for speed reasons


class CRC16 {
 public:
   enum {
      // CRC-16 polynom: x^16 + x^15 + x^2 + x^0
      CRC_16 = 0x18005, // works for audio mpeg 
      // CCITT  polynom: x^16 + x^12 + x^5 + x^0
      CCITT  = 0x11021  // (untested)
   };
   
   // create crc engine and reset to 0 (build shift table)
   CRC16(unsigned int polynom);
   
   // reset engine to init
   void reset(unsigned short init = 0) { c = init; }
   
   // add 8 bits
   void add(unsigned char b) { c = tab[(c>>8)^b] ^ (c<<8); }
   
   // get current crc value
   unsigned short crc() const { return c; }
   
 private:            
   // private data
   unsigned short tab[256];  // shift table
   unsigned short c;         // current crc value
};


