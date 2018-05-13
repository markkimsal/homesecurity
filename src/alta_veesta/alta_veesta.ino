/**
 *
 */
#include "alta_veesta.h"
#include "util.h"
#include <avr/interrupt.h>
#include <avr/power.h> //for F_CPU


#ifdef HAVE_NETWORK
#include <utility/w5100.h>
#include "api_call.h"
#include <SPI.h>
#include <Ethernet.h>

#endif

#ifdef HAVE_OLED
#include "oled.h"
#endif

int msg_len_status = 45;
int msg_len_ack = 2;

volatile int  seq_poll = 0;

char expect_byt           = NULL;
void (*expect_callback_error)(void *data) = NULL;
void (*expect_callback_complete)(void *data) = NULL;

char combuf[30];
int  combufidx = 0;
bool reading_command = false;

char gcbuf[100];
int  gidx = 0;
int  lastgidx = 0;


char alarm_buf[3][30];

// Used to read bits on F7 message
int const BIT_MASK_BYTE1_BEEP = 0x07;

int const BIT_MASK_BYTE2_ARMED_HOME = 0x80;
int const BIT_MASK_BYTE2_LOW_BAT    = 0x40;
int const BIT_MASK_BYTE2_READY      = 0x10;

int const BIT_MASK_BYTE3_CHIME_MODE = 0x20;
int const BIT_MASK_BYTE3_BYPASS     = 0x10;

int const BIT_MASK_BYTE3_AC_POWER   = 0x08;
int const BIT_MASK_BYTE3_ARMED_AWAY = 0x04;

volatile unsigned long low_time = 0;
bool   mid_msg = false;
bool   mid_ack = false;

struct trouble {
	int code;
	short qual;
	short zone;
	short user;
};

typedef struct trouble Trouble;

#include "out_wire.h"

SoftwareSerial vista(RX_PIN, TX_PIN, true);

int ledPin =  13;    // LED connected to digital pin 13
int syncBuf = 0;
int vistaBaud = 4800;


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


void blink_alive() {

	for (int x=0; x < 6; x++) {
		digitalWrite(ledPin, HIGH);
		delay(100);
		digitalWrite(ledPin, LOW);
		delay(100);
	}
}

void blink_dhcp() {

	for (int x=0; x < 6; x++) {
		digitalWrite(ledPin, HIGH);
		delay(50);
		digitalWrite(ledPin, LOW);
		delay(50);
	}
}

void expect_response(char byt) {

	expect_byt = byt;
}

void clear_expect() {
	expect_byt               = NULL;
	expect_callback_error    = NULL;
	expect_callback_complete = NULL;
}

void on_response_error( void (*func)(void *data) ) {
	expect_callback_error = func;
}

void on_response_complete( void (*func)(void *data) ) {
	expect_callback_complete = func;
}


/**
 * return false if no error callback was specified
 * true otherwise
 */
bool _on_response_error(void *data) {
	if (expect_callback_error == NULL) {
		return false;
	}
	expect_callback_error(data);
	return true;
}
/**
 * return false if no complete callback was specified
 * true otherwise
 */
bool _on_response_complete(void *data) {
	if (expect_callback_complete == NULL) {
		return false;
	}
	expect_callback_complete(data);
	return true;
}

/**
 * F2,0B,60,6D,0D,45,44,A0,
 *
 *
 * , ,c,m, ,E,C, , ,A,l,a,r,m, ,C,a,n,c,e,l,e,d,1,5,',
 * F2,1F,63,6D,0D,45,43,F5,EC,41,6C,61,72,6D,20,43,61,6E,63,65,6C,65,64,31,35,27,
 *
 */
void read_chars_dyn(char buf[], int *idx, int limit)
{
	char c;
	int  ct = 1;
	int  x  = 0;
	int idxval = *idx;

	digitalWrite(ledPin, HIGH);   // set the LED on

	while (!vista.available()) {
	}

	c = vista.read();

	buf[ idxval ] = c;
	idxval++;

	ct = (int)c;

	while (x < ct) {
		if (vista.available()) {
			c = vista.read();
			if (idxval >= limit) {
				Serial.print("Dyn Buffer overflow: ");
				Serial.println(idxval, DEC);
				*idx = idxval;
				return;
			}
			buf[ idxval ] = c;
			idxval++;
			x++;
		}
	}
	digitalWrite(ledPin, LOW);
	*idx = idxval;
}

void read_chars_single(char buf[], int *idx)
{
	char c;
	int  x=0;
	int idxval = *idx;
	while (!vista.available()) { tunedDelay(1); }

	c = vista.read();
	buf[ idxval ] = c;
	idxval++;
	x++;
	*idx = idxval;
}

/*
returns 1 if there are more chars to read
returns 0 if serial.avaialable() is false
*/
int read_chars(int ct, char buf[], int *idx, int limit)
{
	char c;
	int  x=0;
	int idxval = *idx;
	//digitalWrite(ledPin, HIGH);   // set the LED on
	while (x < ct) {

		if (vista.available()) {
			c = vista.read();
			if (idxval >= limit) {
				*idx = idxval;
				return 1;
			}
			buf[ idxval ] = c;
			idxval++;
			x++;
		}
	}
	//digitalWrite(ledPin, LOW);
	*idx = idxval;
	return 0;
}



void on_alarm() {
	#ifdef HAVE_NETWORK
	api_call_alarm();
	#endif
}

void on_init() {
	#ifdef HAVE_NETWORK
	api_call_init();
	#endif
}


/**
 * This is a panic button hold
 , ,f,f,c, ,E,C, ,1, ,E,l, , , , , , , ,
F2,16,66,66,63,02,45,43,F5,31,FB,45,6C,F5,EC,01,01,01,01,88,
1111-0010,0001-0110,0110-0110,0110-0110,0110-0011,0000-0010,0100-0101,0100-0011,1111-0101,0011-0001,1111-1011,0100-0101,0110-1100,1111-0101,1110-1100,0000-0001,0000-0001,0000-0001,0000-0001,1000-1000,
no alarm

 * This is a door open disarmed
 , ,f,f,c, ,E,C, ,1, ,E,l, , , , , , , ,
F2,16,66,66,63,02,45,43,F5,31,FB,45,6C,F5,EC,01,02,01,06,82,
1111-0010,0001-0110,0110-0110,0110-0110,0110-0011,0000-0010,0100-0101,0100-0011,1111-0101,0011-0001,1111-1011,0100-0101,0110-1100,1111-0101,1110-1100,0000-0001,0000-0010,0000-0001,0000-0110,1000-0010,
no alarm

 , ,f,b,c, ,E,C, ,1, ,E,l, , , , , , , ,
F2,16,66,62,63,02,45,43,F5,31,FB,45,6C,F5,EC,01,02,01,06,86,
1111-0010,0001-0110,0110-0110,0110-0010,0110-0011,0000-0010,0100-0101,0100-0011,1111-0101,0011-0001,1111-1011,0100-0101,0110-1100,1111-0101,1110-1100,0000-0001,0000-0010,0000-0001,0000-0110,1000-0110,
no alarm


 * Closing a door, disarmed
 , ,f,`,c, ,E,C, ,1, ,E,l, , , , , , , ,
F2,16,66,60,63,02,45,43,F5,31,FB,45,6C,F5,EC,01,01,01,01,8E,
1111-0010,0001-0110,0110-0110,0110-0000,0110-0011,0000-0010,0100-0101,0100-0011,1111-0101,0011-0001,1111-1011,0100-0101,0110-1100,1111-0101,1110-1100,0000-0001,0000-0001,0000-0001,0000-0001,1000-1110,
no alarm

 * Follower, stay
 , ,f,d,c, ,E,C, ,1, ,E,l, , , , , , , ,
F2,16,66,64,63,02,45,43,F5,31,FB,45,6C,F5,EC,02,01,01,06,84,
1111-0010,0001-0110,0110-0110,0110-0100,0110-0011,0000-0010,0100-0101,0100-0011,1111-0101,0011-0001,1111-1011,0100-0101,0110-1100,1111-0101,1110-1100,0000-0010,0000-0001,0000-0001,0000-0110,1000-0100,
no alarm

 */
void on_status(char cbuf[], int *idx) {


	#ifdef DEBUG_STATUS
	/*
	Serial.print("F2: {");

	//first 6 bytes are headers
	for (int x = 1; x < 7 ; x++) {
		if (x > 1)
		Serial.print(",");
		print_hex( cbuf[x], 8);
	}
	//7th byte is incremental counter
	Serial.print (" cnt: ");
	print_hex( cbuf[7], 8 );
	Serial.println ("}");

	//8-end is body
	Serial.print("F2: {");
	for (int x = 8; x < *idx ; x++) {
		print_hex( cbuf[x], 8);
		Serial.print(",");
	}

	Serial.println("}");
	*/
	#endif

	#ifdef DEBUG_STATUS
	print_unknown_json( cbuf );
	#endif

	//F2 messages with 18 bytes or less don't seem to have
	// any important information
	if ( 19 > (int) cbuf[1]) {
		#ifdef DEBUG_STATUS
		/*
		Serial.println("F2: Unknown message - too short");
		*/
		#endif
		//print unkonw messages as unknown type and list all bytes

		//clear memory
		memset(cbuf, 0, sizeof(cbuf));
		*idx = 0;
		return;
	}

	//19th spot is 01 for disarmed, 02 for armed
	//short armed = (0x02 & cbuf[19]) && !(cbuf[19] & 0x01);
	short armed = 0x02 & cbuf[19];

	//20th spot is away / stay
	// this bit is really confusing
	// it clearly switches to 2 when you set away mode
	// but it is also 0x02 when an alarm is canceled,
	// but not cleared - even if you are in stay mode.
	short away = 0x02 & cbuf[20];

	//21st spot is for bypass
	short bypass = 0x02 & cbuf[21];

	//22nd spot is for alarm types
	//1 is no alarm
	//2 is ignore faults (like exit delay)
	//4 is a alarm
	//6 is a fault that does not cause an alarm
	//8 is for panic alarm.
	short exit_delay = (cbuf[22] & 0x02);
	short fault      = (cbuf[22] & 0x04);
	short panic     = (cbuf[22] & 0x08);

	//print as JSON
	Serial.print("{\"type\":\"status\"");

	Serial.print(",\"armed\": ");
	if (armed) {
		Serial.print("\"yes\"");
	} else {
		Serial.print("\"no\"");
	}
	Serial.print(", \"exit_delay\": ");
	if (exit_delay) {
		Serial.print("\"yes\"");
	} else {
		Serial.print("\"no\"");
	}
	Serial.print(", \"mode\": ");
	if (away) {
		Serial.print("\"away\"");
	} else {
		Serial.print("\"stay\"");
	}
	Serial.print(", \"ignore_faults\": ");
	if (exit_delay) {
		Serial.print("\"yes\"");
	} else {
		Serial.print("\"no\"");
	}
	Serial.print(", \"faulted\": ");
	if (fault) {
		Serial.print("\"yes\"");
	} else {
		Serial.print("\"no\"");
	}
	Serial.print(", \"panic\": ");
	if (panic) {
		Serial.print("\"yes\"");
	} else {
		Serial.print("\"no\"");
	}

	
	Serial.println("}");

	if ( armed && fault && !exit_delay ) {
		on_alarm();
		Serial.println ("{\"type\": \"alarm\"}");
		//save gcbuf for debugging
		strncpy(alarm_buf[0],  cbuf, 30);
	} else if ( !armed  && fault  && away && !exit_delay) {
		//away bit always flips to 0x02 when alarm is canceled
		Serial.println ("{\"type\": \"cancel\"}");
	} else {
		//Serial.println ("F2: no alarm");
	}

	memset(cbuf, 0, sizeof(cbuf));
	*idx = 0;
}

void on_display(char cbuf[], int *idx) {
    // first 4 bytes are addresses of intended keypads to display this message
    // from left to right MSB to LSB
    // 5th byte represents  ??? (not the zone)
    // 6th binary encoded data including beeps
    // 7th binary encoded data including status armed mode
    // 8th binary encoded data including ac power and chime
    // 9th byte Programming mode = 0x01
    // 10th byte promt position in the display message of the expected input

    // print out message as JSON
    Serial.print("{\"type\":\"display\"");
    for (int x = 0; x <= 10 ; x++) {
        switch ( x ) {
		/*
            case 1:
                Serial.print(",");
                Serial.print(" \"addr1\": \"");
                print_hex( cbuf[x], 8);
                Serial.print("\"");
                break;
            case 2:
                Serial.print(",");
                Serial.print(" \"addr2\": \"");
                print_hex( cbuf[x], 8);
                Serial.print("\"");
                break;
				*/
            case 3:
				if (cbuf[x] & 0x02) {
                    Serial.print(", \"kp17\": \"active\"");
				}
				if (cbuf[x] & 0x04) {
                    Serial.print(", \"kp18\": \"active\"");
				}
				if (cbuf[x] & 0x08) {
                    Serial.print(", \"kp19\": \"active\"");
				}
				if (cbuf[x] & 0x10) {
                    Serial.print(", \"kp20\": \"active\"");
				}
				if (cbuf[x] & 0x20) {
                    Serial.print(", \"kp21\": \"active\"");
				}
				if (cbuf[x] & 0x40) {
                    Serial.print(", \"kp22\": \"active\"");
				}
				if (cbuf[x] & 0x80) {
                    Serial.print(", \"kp23\": \"active\"");
				}
//                print_hex( cbuf[x], 8);
//                Serial.print("\",");
                break;
				/*
            case 4:
                Serial.print(",");
                Serial.print(" \"addr4\": \"");
                print_hex( cbuf[x], 8);
                Serial.print("\"");
                break;
            case 5:
                Serial.print(",");
                Serial.print(" \"zone\": \"");
                print_hex( cbuf[x], 8);
                Serial.print("\"");
                break;
				*/
            case 6:
                if ( (cbuf[x] & BIT_MASK_BYTE1_BEEP ) > 0 ) {
                    Serial.print(",");
                    Serial.print(" \"beep\": \"");
                    print_hex( cbuf[x], 8);
                    Serial.print("\"");
                }
                break;
            case 7:
                if ( (cbuf[x] & BIT_MASK_BYTE2_ARMED_HOME ) ) {
                    Serial.print(", \"ARMED_STAY\": \"true\"");
                } else {
                    Serial.print(", \"ARMED_STAY\": \"false\"");
                }
                if ( (cbuf[x] & BIT_MASK_BYTE2_LOW_BAT ) ) {
                    Serial.print(", \"low_batt\": \"true\"");
                }

                if ( (cbuf[x] & BIT_MASK_BYTE2_READY )) {
                    Serial.print(", \"READY\": \"true\"");
                } else {
                    Serial.print(", \"READY\": \"false\"");
                }
//                print_hex( cbuf[x], 8);
                break;
            case 8:
                if ( (cbuf[x] & BIT_MASK_BYTE3_CHIME_MODE ) ) {
                    Serial.print(", \"chime\": \"on\"");
                } else {
                    Serial.print(", \"chime\": \"off\"");
                }
                if ( (cbuf[x] & BIT_MASK_BYTE3_BYPASS ) ) {
                    Serial.print(", \"bypass_zone\": true");
                }

                if ( (cbuf[x] & BIT_MASK_BYTE3_AC_POWER ) ) {
                    Serial.print(", \"ac_power\": \"on\"");
                } else {
                    Serial.print(", \"ac_power\": \"off\"");
                }
                if ( (cbuf[x] & BIT_MASK_BYTE3_ARMED_AWAY ) > 0 ) {
                    Serial.print(", \"ARMED_AWAY\": \"true\"");
                } else {
                    Serial.print(", \"ARMED_AWAY\": \"false\"");
                }
//                print_hex( cbuf[x], 8);
                break;
				/*
            case 9:
                Serial.print(", \"programming_mode\": \"");
                if ( cbuf[x] == 0x01 ) {
                    print_hex( cbuf[x], 8);
                } else {
                    Serial.print("0");
                }
                Serial.print("\"");
                break;
				*/
            case 10:
                if ( cbuf[x] != 0x00 ) {
                    Serial.print(",\"prompt_pos\": \"");
                    Serial.print( (int)cbuf[x] );
                    print_hex( cbuf[x], 8);
                    Serial.print("\"");
                }

                break;
            default:
//                print_hex( cbuf[x], 8);
//                Serial.print(";");
                break;
            }
	}
	Serial.print(",\"msg\": \"");
	for (int x = 12; x < *idx -1; x++) {
		if ((int)cbuf[x] < 32 || (int)cbuf[x] > 126) {
		    //don't print non printable ascii
//			Serial.print(" ");
		} else {
			Serial.print(cbuf[x]);
		}
		//TODO: fix timing
		//this is because there's a timing bug and we get an extra
		//1 bit in the most significant position
		if (cbuf[x] & 0x80) {
			cbuf[x] = cbuf[x] ^ 0x80;
			Serial.print( cbuf[x] );
		}
	}

	Serial.println("\"}");
		
	//DEBUG
	#ifdef DEBUG_DISPLAY
	print_unknown_json( cbuf , *idx );
	#endif

}

void on_poll() {

	if ( !have_message() ) return;

	uint8_t KPADDR = fetch_kpaddr();

	vista.setParity(false);
	digitalWrite(ledPin, HIGH);
	/*
	digitalWrite(TX_PIN, HIGH);

    uint8_t oldSREG = SREG;
    cli();
	tunedDelay(3000);
	digitalWrite(ledPin, LOW);
	digitalWrite(TX_PIN, LOW);
    SREG = oldSREG;
	sei();
	return;
	*/

/*
	seq_poll++;
	if (seq_poll > 1) {
		seq_poll = 0;
		low_time = 0;
		return;
	}
	*/
    uint8_t oldSREG = SREG;
    cli();

	vista.write(0xff);

	//just burn time waiting for CTS
	//interrupts take too much overhead on 8Mhz
	while(!vista.rx_pin_read()){}
	vista.write(0xff);
	while(!vista.rx_pin_read()){}
	vista.write(kpaddr_to_bitmask(KPADDR));

	vista.tx_pin_write( LOW );
	tunedDelay(601);
	vista.setParity(true);
	digitalWrite(ledPin, LOW);
    SREG = oldSREG;
	sei();
	return;
}

void ack_f7() {

    uint8_t oldSREG = SREG;
    cli();

	uint8_t KPADDR = fetch_kpaddr();

	vista.setParity(false);

	//just burn time waiting for CTS
	//interrupts take too much overhead on 8Mhz
	while(!vista.rx_pin_read()){}
	vista.write(0xff);
	while(!vista.rx_pin_read()){}

	vista.write(0xff);
	while(!vista.rx_pin_read()){}

	vista.write(kpaddr_to_bitmask(KPADDR));

	vista.tx_pin_write( LOW );
	tunedDelay(3000);
	vista.setParity(true);
    SREG = oldSREG;
	sei();
	return;
}

/**
 * Typical packet
 * positions 8 and 9 hold the report code
 * in Ademco Contact ID format
 * the lower 4 bits of 8 and both bites of 9
 * ["F9","43","0B","58","80","FF","FF","18","14","06","01","00","20","90"]}
 *
 * It seems that, for trouble indicators (0x300 range) the qualifier is flipped
 * where 1 means "new" and 3 means "restored"
 * 0x48 is a startup sequence, the byte after 0x48 will be 00 01 02 03
 */
void on_lrr(char cbuf[], int *idx, SoftwareSerial &vista) {

	int len = cbuf[2];

	if (len == 0 ) {
		#ifdef DEBUG_LRR
		print_unknown_json( cbuf , *idx );
		#endif
		return;
	}
	char type = cbuf[3];

	//TODO: check chksum
	char lcbuf[12];
	int  lcbuflen = 0;

	//if we try to transmit too quickly, the rising edge of the
	//yellow line causes a read and delays the sending of all
	//response bytes
	tunedDelay(400);
	Trouble tr;
	tr.code = 0;
	//0x52 means respond with only cycle message
	//0x48 means same thing
	//, i think 0x42 and and 0x58 are the same
	if (type == (char) 0x52
	  || type == (char) 0x48

	) {
		lcbuf[0] = (char)(cbuf[1]);
		lcbuflen++;
	} else if (type == (char) 0x58) {
		//just respond, but 0x58s have lots of info

		tr.qual    = (0xf0 & cbuf[8]) >> 4;
		tr.code    = (0x0f & cbuf[8]) << 8;
		tr.code   |= cbuf[9];
		tr.zone   = cbuf[12] >> 4;
		tr.user   = cbuf[12] >> 4;

		lcbuf[0] = (char)(cbuf[1]);
		lcbuflen++;
	} else {

		lcbuf[0] = (char)((cbuf[1] + 0x40) & 0xFF);
		lcbuf[1] = (char) 0x04;
		lcbuf[2] = (char) 0x00;
		lcbuf[3] = (char) 0x00;
		//0x08 is sent if we're in test mode
		//0x0a after a test
		//0x04 if you have network problems?
		//0x06 if you have network problems?
		lcbuf[4] = (char) 0x00;
		lcbuflen = 5;
		expect_response((char)((cbuf[1] + 0x40) & 0xFF));
	}

	//we don't need a checksum for 1 byte messages (no length bit)
	//if we don't even have a message length byte, then we are just
	// ACKing a cycle header byte.
	if (lcbuflen >= 2) {
		int chksum = 0;
		for (int x=0; x<lcbuflen; x++) {
			chksum += lcbuf[x];
		}
		chksum -= 1;
		chksum = chksum ^ 0xFF;
		lcbuf[lcbuflen] = chksum;
		lcbuflen++;
	}

	for (int x=0; x<lcbuflen; x++) {
		if (!vista.write(lcbuf[x])) {
		Serial.println("ERROR writing byte");
		return;
		}
	}

	#ifdef DEBUG_LRR
	print_unknown_json( cbuf , *idx );
	print_unknown_json( lcbuf , lcbuflen );
	#endif

	//send events in json
	//we must ack first before sending
	//this much serial data
	if (tr.code != 0) {
		Serial.print("{\"type\":\"event\",\"code\":\"");
		print_hex(tr.code, 16);
		Serial.print("\",\"qualifier\":\"");
		//flip qualifier for trouble codes
		if ( (tr.code & 0x0300) == 0x0300) {
			if (tr.qual == 1) {
				Serial.print("new");
			} else {
				Serial.print("restore");
			}

		} else {
			if (tr.qual == 3) {
				Serial.print("new");
			} else {
				Serial.print("restore");
			}
		}
		Serial.print("\",\"user_zone\":");
		Serial.print("\"");
		Serial.print(tr.user);
		Serial.println("\"}");
	}
}


void on_ack(char cbuf[], int *idx, SoftwareSerial &vista) {

	int kpadr = (int)cbuf[1];

	uint8_t KPADDR = fetch_kpaddr();
	#ifdef DEBUG_KEYS
	Serial.print("F6: kp ack addr = ");
	Serial.println(kpadr, DEC);
	Serial.print("F6: bitmask is ");
	Serial.println(kpaddr_to_bitmask(KPADDR), HEX);

	Serial.print("F6: my kpaddr is ");
	Serial.println(KPADDR, DEC);
	#endif

	tunedDelay(210);
	if (kpadr == (int)KPADDR) {
		tunedDelay(600);
		write_chars( vista );
		//clear memroy
//		out_wire_init();
//		memset(cbuf, 0, sizeof(cbuf));
//		*idx = 0;
	}
}


/**
 * Execute a TT+ command
 */
void execute_command() {
	String command = String(combuf);
	if (command.substring(0,3) != "TT+") {
		return;
	}
	String subcommand = command.substring(3, command.indexOf('='));
	String value = command.substring(command.indexOf('=')+1);
	if (subcommand == "KPADDR") {
		if (!store_kpaddr( value.toInt() )) {
		Serial.print("Error: failed to set kpaddr: ");
		Serial.println(value.toInt());
		}
	}
	if (subcommand == "SYSINFO") {
		write_sys_info();
	}

	#ifdef DEBUG
	Serial.print("\nGot command: ");
	Serial.print(subcommand);
	Serial.print(" = ");
	Serial.print(value);
	Serial.print("\n");
	#endif

}

void readConsole() {

	const int termChar = 13; //Terminate lines with CR
	int inByte         = 0;

	if (!Serial.available()) return;


	do {
		inByte = Serial.read();
		if (inByte == termChar) {
			if (combufidx > 0) {
				execute_command();
				memset(combuf, 0, sizeof(combuf));
				combufidx = 0;
			}

			reading_command = false;
			break;
		}
		if (inByte < 32 || inByte > 126) {
			break;
		}
		//if bytes are TT+ then issue special command
		if (inByte == 'T') {
			reading_command = true;
		}
		if (reading_command) {
			combuf[ combufidx ] = inByte;
			combufidx++;
			break;
		}
		out_wire_queue(inByte);
	} while (Serial.available());

	#ifdef DEBUG
		if (!reading_command) {
			debug_out_buf();
		}
	#endif
}


void on_pin_change() {

	if (mid_ack) return;

	//low
	if (low_time && vista.rx_pin_read()) {
		if ( (millis() - low_time) > 9) {
			on_poll();
			low_time=0;
		} else {
			//regular read
			mid_msg = true;
			vista.recv();
			low_time=0;
		}
		return;
	}


	//high
	if (vista.rx_pin_read()) {
		low_time=0;
		vista.recv();
		mid_msg = false;
	} else {
		low_time = millis();
		mid_msg = false;
	}

	if (mid_msg) {
		vista.recv();
		return;
	}

}



void loop()
{

	if (!vista.available()) {
        readConsole();
		return;
	}
	low_time = 0;

	char x;

	x = vista.read();
	if (!x) {
		//print_hex(x,8);
		//Serial.println(".");
		return;
	}
	switch_first_byte(x, vista);

	//clear buffer
	memset(gcbuf, 0, sizeof(gcbuf));
	gidx = 0;

}

void switch_first_byte(int x, SoftwareSerial vista) {

	if (expect_byt != NULL && x != expect_byt) {
		if(!_on_response_error(&x)) {
			Serial.print("{\"type\":\"error\",\"msg\":\"Unexpected byte with no error handler set. ");
			print_hex(x, 8);
			Serial.println("\"}");
		}
		clear_expect();
	}
	if (expect_byt != NULL && x == expect_byt) {
		_on_response_complete(&x);
		clear_expect();
		return;
	}

	//start of new message
	if ((int)x == 0xFFFFFFF7) {

		//store first byte in global char buf
		gcbuf[ gidx ] = x;
		gidx++;

		read_chars( msg_len_status -1, gcbuf, &gidx, 45);
		ack_f7();
		//eat up the remaining 4 pulsing 0x00
		//read_chars( 4, gcbuf, &gidx, 4);

		on_display(gcbuf, &gidx);
		return;
	}

	//key ack
	if ((int)x == 0xFFFFFFF6) {
		//debug_cbuf(gcbuf, &gidx, true);
		memset(gcbuf, 0, sizeof(gcbuf));
		gidx = 0;

		gcbuf[ gidx ] = x;
		gidx++;

		//read_chars( msg_len_ack , gcbuf, &gidx, 30);
		read_chars_single(gcbuf, &gidx);
		mid_ack = true;
		vista.setParity(true);

		on_ack(gcbuf, &gidx, vista);
		mid_ack = false;
		vista.setParity(false);
		return;
	}

	//state change?
	if ((int)x == 0xFFFFFFF2) {
		//debug_cbuf(gcbuf, &gidx, true);

		gcbuf[ gidx ] = x;
		gidx++;

		read_chars_single(gcbuf, &gidx);
		int len = gcbuf[ gidx-1 ];
		read_chars(len, gcbuf, &gidx, 30);
		on_status(gcbuf, &gidx);
		return;
	}

	//Long Range Radio (LRR)
	if ((int)x == 0xFFFFFFF9) {
		memset(gcbuf, 0, sizeof(gcbuf));
		gidx = 0;

		//store first byte in global char buf
		gcbuf[ gidx ] = x;
		gidx++;

		//ready cycle
		read_chars_single(gcbuf, &gidx);
		//read len
		read_chars_single(gcbuf, &gidx);

		read_chars((int)gcbuf[2] , gcbuf, &gidx, 30);

		on_lrr(gcbuf, &gidx, vista);
		return;
	}


	//unknown 0xFF
	if ((int)x == 0xFFFFFFFf) {

		gcbuf[ gidx ] = x;
		gidx++;

		read_chars_single(gcbuf, &gidx);
		int len = gcbuf[ gidx-1 ];
		read_chars(len, gcbuf, &gidx, 30);
		print_unknown_json(gcbuf);
		return;
	}

	if (expect_byt == NULL) {
		Serial.print("{\"type\":\"unknown\",\"byte\":\"");
		print_hex((char)x, 8);
		Serial.println("\"}");
	}

	//moved to top
/*
	if (expect_byt != NULL && x != expect_byt) {
		if(!_on_response_error(&x)) {
			Serial.print("{\"type\":\"error\",\"msg\":\"Unexpected byte with no error handler set. ");
			print_hex(x, 8);
			Serial.println("\"}");
		}
		clear_expect();
	}
	if (expect_byt != NULL && x == expect_byt) {
		_on_response_complete(&x);
		clear_expect();
	}
	*/
}



#ifdef HAVE_NETWORK
void send_email() {

	Serial.println("SMTP opening connection...");

	EthernetClient client;
}
#endif

void setup()   {


	memset(combuf, 0, sizeof(combuf));

	// initialize the digital pin as an output:
	pinMode(ledPin, OUTPUT);
	blink_alive();

	out_wire_init();

	pinMode(A3, OUTPUT);
	digitalWrite(A3, HIGH);
	//initialize USB serial
	Serial.begin(115200);
	Serial.println("\"good morning\"");

	#ifdef HAVE_OLED
	oled_test();
	#endif

	//ethernet
	#ifdef HAVE_NETWORK
	#ifdef USE_DHCP
	Serial.println("Starting ethernet with DHCP... ");
	blink_dhcp();
	if (Ethernet.begin(mac) == 0) {
		Serial.println("Failed to configure Ethernet using DHCP");
		// no point in carrying on, so do nothing forevermore:
		for(;;)
		;
	}
	//3 second timeout
	W5100.setRetransmissionTime(0x07D0);
	W5100.setRetransmissionCount(3);

	Serial.print("Got DHCP IP: ");
	Serial.println(Ethernet.localIP());
	#else
	Serial.print("Starting ethernet with static IP... ");
	Ethernet.begin(mac, STATIC_IP);
	Serial.println(Ethernet.localIP());
	#endif
	// print your local IP address:
	Serial.println("Triggering fake alarm for testing... ");
	on_init();

	#ifdef USE_SMTP
//		send_email();
	#endif

	#ifdef USE_HTTP
//		api_call_register_self();
	#endif

	#ifdef USE_ZMQ
//		api_call_register_self();
	#endif
	#endif
	Serial.println(F("\"Starting vista keypad bus\"\n"));

	Serial.print("\"Using kpaddr: ");
	Serial.print((uint8_t)fetch_kpaddr(), DEC);
	Serial.println("\"");

	vista.begin( vistaBaud );
}

//Pin Change INTerrupts
#if defined(PCINT0_vect)
ISR(PCINT0_vect)
{
  on_pin_change();
}
#endif

#if defined(PCINT1_vect)
ISR(PCINT1_vect, ISR_ALIASOF(PCINT0_vect));
#endif

#if defined(PCINT2_vect)
ISR(PCINT2_vect, ISR_ALIASOF(PCINT0_vect));
#endif

#if defined(PCINT3_vect)
ISR(PCINT3_vect, ISR_ALIASOF(PCINT0_vect));
#endif
