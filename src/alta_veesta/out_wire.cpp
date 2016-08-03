#include "config.h"
#include "api_call.h"

#include "SoftwareSerial2.h"

#include "util.h"

/* static */ 
inline void tunedDelay(uint16_t delay) { 
  uint8_t tmp=0;

  asm volatile("sbiw    %0, 0x01 \n\t"
    "ldi %1, 0xFF \n\t"
    "cpi %A0, 0xFF \n\t"
    "cpc %B0, %1 \n\t"
    "brne .-10 \n\t"
    : "+r" (delay), "+a" (tmp)
    : "0" (delay)
    );
}

int  outbufIdx = 0;
char outbuf[20];

int seq = 1;

void out_wire_init() {
	//clear outbuf buffer
	memset(outbuf,0,sizeof(outbuf));
	outbufIdx = 0;
}
void out_wire_queue(char byt ) {
	outbuf[outbufIdx] = byt; // Save the data in a character array
	outbufIdx++; //Increment position in array
}
void ask_for_write(
		SoftwareSerial &vistaSerial
){

	vistaSerial.write(0xff);
	vistaSerial.write(0xff);
	vistaSerial.write(0xfb);
}

bool have_message() {
	return (outbufIdx > 0);
}

/**
 * Send 0-9, # and * characters
 * 1,2,3,4,5,6,7,8,9,0,#,*,
 * 31,32,33,34,35,36,37,38,39,30,23,2A,
 * 0011-0001,0011-0010,0011-0011,0011-0100,0011-0101,0011-0110,0011-0111,0011-1000,0011-1001,0011-0000,0010-0011,0010-1010,
 *
 */
void write_chars(
		SoftwareSerial &vistaSerial
){

	if (outbufIdx == 0) {return;}

	int header = ((++seq << 6) & 0xc0) | (18 & 0x3F);


	//header is the bit mask YYXX-XXXX
	//	where YY is an incrementing sequence number
	//	and XX-XXXX is the keypad address (decimal 16-31)
	//int header = 18 & 0x3F;

	vistaSerial.write((int)header);
	vistaSerial.flush();

	vistaSerial.write((int) outbufIdx +1);
	vistaSerial.flush();

	/*
	print_hex((int)header, 8);
	Serial.println();
	*/
	//Serial.println(outbufIdx, DEC);
	//adjust characters to hex values.
	//ASCII numbers get translated to hex numbers
	//# and * get converted to 0xA and 0xB
	// send any other chars straight, although this will probably 
	// result in errors
	int checksum = 0;
	for(int x =0; x < outbufIdx; x++) {
		if (outbuf[x] >= 0x30 && outbuf[x] <= 0x39) {
			outbuf[x] -= 0x30;
			checksum += outbuf[x];

			vistaSerial.write((int)outbuf[x]);
			vistaSerial.flush();
		}
		if (outbuf[x] == 0x23) {
			outbuf[x] = 0x0B;
			checksum += outbuf[x];
			vistaSerial.write((int)outbuf[x]);
			vistaSerial.flush();
		}
		if (outbuf[x] == 0x2A) {
			outbuf[x] = 0x0A;
			checksum += outbuf[x];
			vistaSerial.write((int)outbuf[x]);
			vistaSerial.flush();
		}
	}

	/*
	print_hex((int)outbufIdx + 1, 8);
	for(int x =0; x < outbufIdx; x++) {
	print_hex((int)outbuf[x], 8);
	}
	*/
	int chksum = 0x100 - (header + checksum + (int)outbufIdx+1);

	vistaSerial.write((int) (0x100-(header+checksum+ outbufIdx+1)) );
	vistaSerial.flush();

	//print_hex((int)chksum, 8);
    //Serial.println();
}

void debug_out_buf()
{
	Serial.println();
	Serial.write("Output buffer: ");
	for(int x =0; x < outbufIdx; x++) {
		Serial.write((int)outbuf[x]);
	}
	Serial.println();
	Serial.flush();
}


