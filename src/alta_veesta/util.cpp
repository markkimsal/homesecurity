#include "alta_veesta.h"

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

