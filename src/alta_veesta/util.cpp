#include "alta_veesta.h"
#include <EEPROM.h>

/**
 * First 64 bytes are reserved
 */
#define KPADDR_EEPROM_OFFSET 64

/**
 * Load KPADDR from EEPROM
 * Use 16 if no address has been stored
 */
uint8_t fetch_kpaddr() {
	int kpaddr = EEPROM.read(KPADDR_EEPROM_OFFSET);
	if (kpaddr == 0) {
		return 16;
	}
	return kpaddr;
}

/**
 * Store KPADDR to EEPROM
 */
bool store_kpaddr(int addr) {
	if (addr < 16 || addr > 25) {
		return false;
	}
	EEPROM.write(KPADDR_EEPROM_OFFSET, addr);
	return true;
}

void write_sys_info() {
	Serial.print(F("{\"type\":\"sysinfo\",\"version\":\""));
	Serial.print(VERSION);
	Serial.print(F("\",\"kpaddr\":\""));
	Serial.print(fetch_kpaddr(), DEC);
	Serial.println(F("\"}"));
}

/**
 * 16 is a valid address, but not for keypads
 * I think it's for wireless or some dialer / uploader
 * Let's inverse for the serial output too.
 */
char kpaddr_to_bitmask(int kpaddr) {
	return 0xFF ^ (0x01 << (kpaddr - 16));
}

void print_hex(int v, int num_places)
{
    int mask=0, n, num_nibbles, digit;

    for (n=1; n<=num_places; n++)
    {
        mask = (mask << 1) | 0x0001;
    }
    v = v & mask; // truncate v to specified number of places

    num_nibbles = num_places / 4;
    if ((num_places % 4) != 0)
    {
        ++num_nibbles;
    }

    do
    {
        digit = ((v >> (num_nibbles-1) * 4)) & 0x0f;
        Serial.print(digit, HEX);
    } while(--num_nibbles);

}

void print_binary(int v, int num_places)
{
    int mask=0, n;

    for (n=1; n<=num_places; n++)
    {
        mask = (mask << 1) | 0x0001;
    }
    v = v & mask;  // truncate v to specified number of places

    while(num_places)
    {

        if (v & (0x0001 << num_places-1))
        {
             Serial.print("1");
        }
        else
        {
             Serial.print("0");
        }

        --num_places;
        if(((num_places%4) == 0) && (num_places != 0))
        {
            Serial.print("-");
        }
    }
}


void debug_cbuf(char cbuf[], int *idx, bool clear) 
{
	if (*idx == 0) {
		return;
	}

	Serial.println();

	//print printable ASCII
	for(int x =0; x < *idx; x++) {
		if ((int)cbuf[x] < 32 || (int)cbuf[x] > 126) {
			Serial.print("  ");
			//Serial.print(cbuf[x], DEC);
		} else {
//			Serial.print( " ");
			Serial.print( cbuf[x] );
		}
		Serial.print(",");
	}
	Serial.println();

	//print HEX
	for(int x =0; x < *idx; x++) {
		print_hex( cbuf[x], 8);
		Serial.print(",");
	}
	Serial.println();

	//print binary
	for(int x = 0; x < *idx; x++) {
		print_binary( cbuf[x], 8);
		Serial.print(",");
	}
	Serial.println();


/*
	Serial.print("size of cbuf: ");
	Serial.println(sizeof(cbuf), DEC);
	Serial.print("idx: ");
	Serial.println(*idx, DEC);
*/

	if (clear) {
		memset(cbuf, 0, sizeof(cbuf));
		*idx = 0;
	}
}


void print_unknown_json(char cbuf[], int len=-1) {

	Serial.print("{\"type\":\"debug\",\"bytes\":[");
	//read from packet second position
	//add in the first byte and the len byte (+2)
	if (len == -1) {
		len = (int) cbuf[1]+2;
	}

	//length is for the bytes after the 2 byte header
	for (int x = 0; x < len ; x++) {
		if (x > 0)
		Serial.print(",");
		Serial.print("\"");
		print_hex( cbuf[x], 8 );
		Serial.print("\"");
	}
	Serial.println("]}");
}

