/*  SHA-1 - an implementation of the Secure Hash Algorithm in C
    HMAC-SHA-1 - an implementation of the HMAC message authentication
    Version as of March 4th 2007
    
    Copyright (C) 2006-2007 CHZ-Soft, Christian Zietz, <czietz@gmx.net>
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

#include <stdint.h>
#include "sha1.h"
#include "hmac.h"

int main(void) {
 
 // SHA1 example
 // initialize structures
 SHA1Init();
 // hash "abc", sample #1 from FIPS 180
 SHA1Block("abc",3);
 // make digest
 SHA1Done();
 // shadigest now contains the digest
 
 // HMAC example, sample #2 from FIPS 198
 // initialize structures with key
 HMACInit("0123456789:;<=>?@ABC",20);
 // hash "a"
 HMACBlock("Sample #2",9);
 // make digest
 HMACDone();
 // hmacdigest now contains the digest 
}