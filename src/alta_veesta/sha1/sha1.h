/*  SHA-1 - an implementation of the Secure Hash Algorithm in C
    Version as of September 22nd 2006
    
    Copyright (C) 2006 CHZ-Soft, Christian Zietz, <czietz@gmx.net>
    See README file for more information.
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the 
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
    Boston, MA  02110-1301, USA
*/

#ifndef __SHA1_H__
#define __SHA1_H__

#include <stdint.h>

extern unsigned char shadigest[20];

void SHA1Init(void);
void SHA1Block(const unsigned char* data, const uint8_t len);
void SHA1Done(void);
void SHA1Once(const unsigned char* data, int len);

#endif

