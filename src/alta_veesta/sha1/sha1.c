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

// define this on little endian architectures
#define SHA_LITTLE_ENDIAN

// define this if you want to process more then 65535 bytes
/* #define SHA_BIG_DATA */

#include <stdint.h>
#include <string.h>

// initial values
#define init_h0  0x67452301
#define init_h1  0xEFCDAB89
#define init_h2  0x98BADCFE
#define init_h3  0x10325476
#define init_h4  0xC3D2E1F0

// bit-shift-operation
#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

union _message {
  unsigned char data[64];
  uint32_t w[16];
} message;

struct shastate {
  uint32_t h0,h1,h2,h3,h4;
#ifdef SHA_BIG_DATA  
  uint32_t count;
#else  
  uint16_t count;
#endif  
};

union _digest {
  unsigned char data[20];
  struct shastate state;
} shadigest;

// processes one endianess-corrected block, provided in message
void SHA1(void) {
  uint8_t i;
  uint32_t a,b,c,d,e,f,k,t;
  
  a = shadigest.state.h0;
  b = shadigest.state.h1;
  c = shadigest.state.h2;
  d = shadigest.state.h3;
  e = shadigest.state.h4;  
  
  // main loop: 80 rounds
  for (i=0; i<=79; i++) {
    if (i<=19) {
      f = d ^ (b & (c ^ d));
      k = 0x5A827999;
    } else if (i<=39) {
      f = b ^ c ^ d;
      k = 0x6ED9EBA1;
    } else if (i<=59) {
      f = (b & c) | (d & (b | c));
      k = 0x8F1BBCDC;
    } else {
      f = b ^ c ^ d;
      k = 0xCA62C1D6;
    }

    // blow up to 80 dwords while in the loop, save some RAM  
    if (i>=16) {
      t = rol(message.w[(i+13)&15] ^ message.w[(i+8)&15] ^ message.w[(i+2)&15] ^ message.w[i&15], 1);
      message.w[i&15] = t;
    }
    
    t = rol(a, 5) + f + e + k + message.w[i&15];
    e = d;
    d = c;
    c = rol(b, 30);
    b = a;
    a = t; 
  }
  
  shadigest.state.h0 += a;
  shadigest.state.h1 += b;
  shadigest.state.h2 += c;
  shadigest.state.h3 += d;
  shadigest.state.h4 += e;
}

void SHA1Init(void) {
  shadigest.state.h0 = init_h0;
  shadigest.state.h1 = init_h1;
  shadigest.state.h2 = init_h2;
  shadigest.state.h3 = init_h3;
  shadigest.state.h4 = init_h4;
  shadigest.state.count = 0;
}

// Hashes blocks of 64 bytes of data.
// Only the last block *must* be smaller than 64 bytes.
void SHA1Block(const unsigned char* data, const uint8_t len) {
  uint8_t i;
  
  // clear all bytes in data block that are not overwritten anyway
  for (i=len>>2;i<=15;i++) {
    message.w[i] = 0;
  }
  
#ifdef SHA_LITTLE_ENDIAN
  // swap bytes
  for (i=0;i<len;i+=4) {
    message.data[i] = data[i+3];
    message.data[i+1] = data[i+2];
    message.data[i+2] = data[i+1];
    message.data[i+3] = data[i];    
  }
#else
  memcpy(message.data, data, len);
#endif  

  // remember number of bytes processed for final block
  shadigest.state.count += len;

  if (len<64) {
    // final block: mask bytes accidentally copied by for-loop
    // and do the padding
    message.w[len >> 2] &= 0xffffffffL << (((~len & 3)*8)+8);
    message.w[len >> 2] |= 0x80L << ((~len & 3)*8);
    // there is space for a qword containing the size at the end
    // of the block
    if (len<=55) {
      message.w[15] = (uint32_t)(shadigest.state.count) * 8;
    }
  }
  
  SHA1();
  
  // was last data block, but there wasn't space for the size: 
  // process another block
  if ((len>=56) && (len<64)) {
    for (i=0; i<=14; i++) {
      message.w[i] = 0;
    }
    message.w[15] = (uint32_t)(shadigest.state.count) * 8;
    SHA1();
  }
}

// Correct the endianess if needed  
void SHA1Done(void) {
#ifdef SHA_LITTLE_ENDIAN
  uint8_t i;
  unsigned char j;
  // swap bytes
  for (i=0;i<=4;i++) {
    j = shadigest.data[4*i];
    shadigest.data[4*i] = shadigest.data[4*i+3];
    shadigest.data[4*i+3] = j;
    j = shadigest.data[4*i+1];
    shadigest.data[4*i+1] = shadigest.data[4*i+2];
    shadigest.data[4*i+2] = j;  
  }
#endif 
}

// Hashes just one arbitrarily sized chunk of data
void SHA1Once(const unsigned char* data, int len) {
  SHA1Init();
  while (len>=0) {
    SHA1Block(data, len>64?64:len);
    len -= 64;
    data += 64;
  }
  SHA1Done();
}



  
