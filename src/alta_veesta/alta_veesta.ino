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

byte mac[] = MAC_ADDR;
#endif

#ifdef HAVE_OLED
#include "oled.h"
#endif


int msg_len_status = 45;
int msg_len_ack = 2;

volatile int  seq_poll = 0;

char gcbuf[30];
int  gidx = 0;
int  lastgidx = 0;


char alarm_buf[3][30];

char guibuf[100];
int  guidx = 0;

// Used to read bits on F7 message
int const BIT_MASK_BYTE1_BEEP = 0x03;

int const BIT_MASK_BYTE2_ARMED_HOME = 0x80;
int const BIT_MASK_BYTE2_READY = 0x10;

int const BIT_MASK_BYTE3_CHIME_MODE = 0x20;
int const BIT_MASK_BYTE3_AC_POWER = 0x08;
int const BIT_MASK_BYTE3_ARMED_AWAY = 0x04;

volatile unsigned long low_time = 0;
bool   mid_msg = false;
bool   mid_ack = false;

#include "out_wire.h"


#include "SoftwareSerial2.h"
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



void on_command(char cbuf[], int *idx) {

	#ifndef DEBUG

	//first 5 bytes are headers
	for (int x = 6; x < *idx -1; x++) {
		Serial.print ( cbuf[x] );
	}

	Serial.println();
	memset(cbuf, 0, sizeof(cbuf));
	*idx = 0;

	#else

	//DEBUG
	debug_cbuf(cbuf, idx, true);

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


	#if DEBUG_STATUS
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
	#endif

	//F2 messages with 18 bytes or less don't seem to have
	// any important information
	if ( 19 > (int) cbuf[1]) {
		#if DEBUG_STATUS
		Serial.println("F2: Unknown message - too short");
		#endif

		//clear memory
		memset(cbuf, 0, sizeof(cbuf));
		*idx = 0;
		return;
	}

	#if DEBUG_STATUS
	//19, 20, 21, 22
	Serial.print("F2: 19 = ");
	print_hex(cbuf[19], 8);
	Serial.println();

	Serial.print("F2: 20 = ");
	print_hex(cbuf[20], 8);
	Serial.println();

	Serial.print("F2: 21 = ");
	print_hex(cbuf[21], 8);
	Serial.println();

	Serial.print("F2: 22 = ");
	print_hex(cbuf[22], 8);
	Serial.println();
	#endif


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
    // 5th byte represent zone
    // 6th binary encoded data including beeps
    // 7th binary encoded data including status armed mode
    // 8th binary encoded data including ac power and chime
    // 9th byte Programming mode = 0x01
    // 10th byte promt position in the display message of the expected input
	#ifdef DEBUG_DISPLAY
    Serial.print("F7: {");
    for (int x = 1; x <= 11 ; x++) {
         print_hex( cbuf[x], 8);
         Serial.print(",");
	}

	Serial.print (" chksm: ");
	print_hex( cbuf[*idx-1], 8 );
	Serial.println ("}");

	//12-end is the body
	Serial.print("F7: ");
	for (int x = 12; x < *idx -1; x++) {
		Serial.print ( cbuf[x] );
	}
	Serial.println();
	#endif

    // print out message as JSON
    Serial.print("{\"type\":\"display\"");
    for (int x = 0; x <= 10 ; x++) {
        switch ( x ) {
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
	debug_cbuf(cbuf, idx, false);
	#endif

	memset(cbuf, 0, sizeof(cbuf));
	*idx = 0;
}

void on_poll() {

	if ( !have_message() ) return;
	vista.setParity(false);

	seq_poll++;
	if (seq_poll > 1) {
		seq_poll = 0;
		low_time = 0;
		return;
	}
    uint8_t oldSREG = SREG;
    cli();

	vista.write(0xff);


	//TODO for 16Mhz these times just seem made up
	if (F_CPU == 16000000)  {
		tunedDelay(1100);
	}else{
		tunedDelay(540);
	}

	vista.write(0xff);


	if (F_CPU == 16000000)  {
		tunedDelay(1150);
	} else {
		tunedDelay(575);
	}


	//keypad addr 18 appears to be 0xF7
	//or 1111-0111

	//keypad addr 20 appears to be 0xFB
	//or 1111-1011

	//keypad addr 19 appears to be 0xFD
	//or 1111-1101

	//keypad addr 18 appears to be 0xFE
	//or 1111-1110


	//vista.write(0xFE);
	//vista.write(0x7F);
	//adjust timing until kpaddr 20 works
	//0xEF should be 20

	//new tests
	//with 236*4
	//0xFE is kpaddr 18 (1111-1110)
	//0xFD is kpaddr 19 (1111-1101)

	//with 1000
	//0xFE is kpaddr 18 (1111-1110)
	//0xFD is kpaddr 18 (1111-1101)
	//0xFB is kpaddr 18 (1111-1011)

	//with 236
	//0xFE broke
	//0xFB is kpaddr 18 (1111-1011)
	//0xEF broke

	//with 236 * 2
	//0xFB is kpaddr 18
	//0xFD is kpaddr 18
	//0xEF broke

	//with tunedDelay(208*5);
	//0x7f is 18
	//0xBf is 18
	//0xDf is 18
	//0xEF doesn't work
    //0xFD is 17;
	vista.write(kpaddr_to_bitmask(KPADDR));

	vista.tx_pin_write( LOW );
	tunedDelay(601);
	vista.setParity(true);
    SREG = oldSREG;
	sei();
	return;
}

void ack_f7() {

    uint8_t oldSREG = SREG;
    cli();

	vista.setParity(false);

	//TODO for 16Mhz these times just seem made up
	if (F_CPU == 16000000)  {
		tunedDelay(590);
	} else {
		tunedDelay(100);
	}
	vista.write(0xff);

	vista.write(0xff);

	vista.write(kpaddr_to_bitmask(KPADDR));

	vista.tx_pin_write( LOW );
	tunedDelay(3000);
	vista.setParity(true);
    SREG = oldSREG;
	sei();
	return;
}


void on_ack(char cbuf[], int *idx, SoftwareSerial &vista) {

	int kpadr = (int)cbuf[1];

	#if DEBUG_KEYS
	Serial.print("F6: kp ack addr = ");
	Serial.println(kpadr, DEC);
	Serial.print("bitmask is ");
	Serial.println(kpaddr_to_bitmask(KPADDR), HEX);
	#endif

	tunedDelay(210);
	if (kpadr == (int)KPADDR) {
		tunedDelay(600);
		write_chars( vista );
		//clear memroy
		out_wire_init();
		memset(cbuf, 0, sizeof(cbuf));
		*idx = 0;
	}
}

void readConsole() {

	const int termChar = 13; //Terminate lines with CR
	int inByte         = 0;

	if (!Serial.available()) return;


	//If we see data (inByte > 0) and that data isn't a carriage return
	do {
		inByte = Serial.read();
		if (inByte == termChar) break;
		if (inByte < 32 || inByte > 126) {
			break;
		}
		out_wire_queue(inByte);
	} while (Serial.available());

	#ifdef DEBUG
		debug_out_buf();
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

	//start of new message
	if ((int)x == 0xFFFFFFF7) {
		guibuf[ guidx ] = x;
		guidx++;

		read_chars( msg_len_status -1, guibuf, &guidx, 45);
		ack_f7();
		//eat up the remaining 4 pulsing 0x00
		read_chars( msg_len_status -1, guibuf, &guidx, 4);


		on_display(guibuf, &guidx);
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

	//state chage?
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

Serial.print("Unknown char: ");
print_hex((char)x, 8);
Serial.println();
}



#ifdef HAVE_NETWORK
void send_email() {

	Serial.println("SMTP opening connection...");

	EthernetClient client;
}
#endif

void setup()   {                


	// initialize the digital pin as an output:
	pinMode(ledPin, OUTPUT);     
	blink_alive();

	out_wire_init();

	pinMode(A3, OUTPUT);
	digitalWrite(A3, HIGH);
	//initialize USB serial
	Serial.begin(115200);
	Serial.println("good morning");

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

	Serial.println("Starting vista keypad bus"); 
	vista.begin( vistaBaud );
}

#if defined(PCINT0_vect)
ISR(PCINT0_vect)
{
  on_pin_change();
}
#endif

#if defined(PCINT1_vect)
ISR(PCINT1_vect)
{
  on_pin_change();
}
#endif

#if defined(PCINT2_vect)
ISR(PCINT2_vect)
{
  on_pin_change();
}
#endif
#if defined(PCINT3_vect)
ISR(PCINT3_vect)
{
  on_pin_change();
}
#endif

