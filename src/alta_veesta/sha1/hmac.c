/*  HMAC-SHA-1 - an implementation of the HMAC message authentication
    Version as of March 4th 2007
    
    Copyright (C) 2007 CHZ-Soft, Christian Zietz, <czietz@gmx.net>
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
#include <string.h>
#include "sha1.h"

#define SHA1_DIGESTSIZE  20
#define SHA1_BLOCKSIZE   64

unsigned char hmackey[SHA1_BLOCKSIZE];

// Initializes HMAC algorithm with given key
// key must be smaller than 64 bytes
void HMACInit(const unsigned char* key, const uint8_t len) {
  uint8_t i;
  
  // copy key, XOR it for the inner digest, pad it to block size
  for (i=0;i<len;i++) {
    hmackey[i] = key[i] ^ 0x36;
  }
  for (i=len;i<SHA1_BLOCKSIZE;i++) {
    hmackey[i] = 0x36;
  }
  
  // initialize SHA1 and hash key
  SHA1Init();
  SHA1Block(hmackey, SHA1_BLOCKSIZE);
}

// Authenticates blocks of 64 bytes of data.
// Only the last block *must* be smaller than 64 bytes.
void HMACBlock(const unsigned char* data, const uint8_t len) {
  SHA1Block(data, len);
}

// Calculates the MAC, hmacdigest will contain the result
// Assumes that the last call to HMACBlock was done with len<64
void HMACDone(void) {
  uint8_t i;
  unsigned char temp[SHA1_DIGESTSIZE];
  
  // terminate inner digest and store it
  SHA1Done();
  memcpy(temp, shadigest, SHA1_DIGESTSIZE);
  
  // prepare key for outer digest
  // buffer will contain the original key xor 0x5c
  for (i=0;i<SHA1_BLOCKSIZE;i++) {
    hmackey[i] ^= 0x6a;
  }
  
  // initialize SHA1 and hash key
  SHA1Init();
  SHA1Block(hmackey, SHA1_BLOCKSIZE);
  // hash inner digest and terminate hash
  SHA1Block(temp, SHA1_DIGESTSIZE);
  SHA1Done();
}

// Authenticates just one arbitrarily sized chunk of data
void HMACOnce(const unsigned char* key, const uint8_t klen, 
              const unsigned char* data, int len) {
  HMACInit(key, klen);
  while (len>=0) {
    HMACBlock(data, len>SHA1_BLOCKSIZE?SHA1_BLOCKSIZE:len);
    len -= SHA1_BLOCKSIZE;
    data += SHA1_BLOCKSIZE;
  }
  HMACDone();
}



  
