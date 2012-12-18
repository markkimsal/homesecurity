/**
 * 
 */

int vistaIn = 8;
int vistaOut = 3;
int msg_len_status = 45;
int msg_len_ack = 4;


char gcbuf[30];
int  gidx = 0;
int  lastgidx = 0;

char alarm_buf[3][30];

char guibuf[100];
int  guidx = 0;

#include "api_call.h"


#include "alta_veesta.h"
#include "config.h"


#include "SoftwareSerial2.h"
SoftwareSerial vista(vistaIn, vistaOut, true);

#include <SPI.h>
#include <Ethernet.h>

byte mac[] = { 0xB8, 0x70, 0xF4, 0x1E, 0xC5, 0x7E };
//byte mac[] = { 0xF6, 0x75, 0x92, 0x78, 0x67, 0x3F };
//b8:70:f4:1e:c5:7e


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

	for (int x=0; x < 3; x++) {
		digitalWrite(ledPin, HIGH);
		delay(200);
		digitalWrite(ledPin, LOW);
		delay(200);
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
			Serial.print(" ");
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
	int  x=0;
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

void read_chars(int ct, char buf[], int *idx, int limit) 
{
	char c;
	int  x=0;
	int idxval = *idx;
	digitalWrite(ledPin, HIGH);   // set the LED on  
	while (x < ct) {
		if (vista.available()) {
			c = vista.read();
			if (idxval >= limit) {
				
				Serial.print("Buffer overflow: ");
				Serial.println(idxval, DEC);
				Serial.println(limit, DEC);
				*idx = idxval;
				return;
			}
			buf[ idxval ] = c;
			idxval++;
			x++;
			//tunedDelay(430);
		}
	}
	digitalWrite(ledPin, LOW); 
	*idx = idxval;
}



void on_alarm() {
	api_call_alarm();
}

void on_init() {
	api_call_init();
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


	//this might be a mistaken read...
	//also, F2 00 should read no more bytes
	//F2 00  are display messages
	if ( cbuf[1] == 0x00) {
	//	return;
	}

	Serial.print("F2: {");
	//first 6 bytes are headers
	for (int x = 1; x < 7 ; x++) {
		print_hex( cbuf[x], 8);
		Serial.print(",");
	}
	//7th byte is incremental counter
	Serial.print (" cnt: ");
	print_hex( cbuf[7], 8 );
	Serial.println ("}");

	//8-end is body
	Serial.print("F2: ");
	for (int x = 8; x < *idx ; x++) {
		print_hex( cbuf[x], 8);
		Serial.print(",");
	}

	Serial.println();

	//F2 messages with less than 16 bytes don't seem to have
	// any important information
	if ( 19 > (int) cbuf[1]) {
		#ifdef DEBUG_STATUS
		Serial.println("F2: Unknown message - too short");
		#endif

		//clear memory
		memset(cbuf, 0, sizeof(cbuf));
		*idx = 0;
		return;
	}

	#ifdef DEBUG_STATUS
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
	short exit_delay = (cbuf[22] & 0x02);
	short fault = (cbuf[22] & 0x04);

	Serial.print("F2: armed: ");
	if (armed) {
		Serial.print("yes");
	} else {
		Serial.print("no");
	}
	Serial.print("; mode: ");
	if (away) {
		Serial.print("away");
	} else {
		Serial.print("stay");
	}
	Serial.print("; ignore faults: ");
	if (exit_delay) {
		Serial.print("yes");
	} else {
		Serial.print("no");
	}
	Serial.print("; faulted: ");
	if (fault) {
		Serial.print("yes");
	} else {
		Serial.print("no");
	}


	
	Serial.println();

	if ( armed && fault && !exit_delay ) {
		on_alarm();
		Serial.println ("F2: ALARM!");
		//save gcbuf for debugging
		strncpy(alarm_buf[0],  cbuf, 30);
	} else if ( !armed  && fault  && away && !exit_delay) {
		//away bit always flips to 0x02 when alarm is canceled
		Serial.println ("F2: alarm canceled");
	} else {
		Serial.println ("F2: no alarm");
	}

	//#ifdef DEBUG_STATUS
	//DEBUG
	//debug_cbuf(cbuf, idx, false);
	//#endif


	memset(cbuf, 0, sizeof(cbuf));
	*idx = 0;
}

void on_display(char cbuf[], int *idx) {

	Serial.print("F7: {");
	for (int x = 1; x < 11 -1; x++) {
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

	//DEBUG
	#ifdef DEBUG_DISPLAY
	debug_cbuf(cbuf, idx, false);

	#endif

	memset(cbuf, 0, sizeof(cbuf));
	*idx = 0;
}

void on_ack(char cbuf[], int *idx) {


	int kpadr = (int)cbuf[1];

	Serial.print("F6: kp ack addr = ");
	Serial.println(kpadr, DEC);


	#ifdef DEBUG_KEYS
	//DEBUG
	debug_cbuf(cbuf, idx, false);
	#endif

	//clear memroy
	memset(cbuf, 0, sizeof(cbuf));
	*idx = 0;

}

void on_debug() {
	int tmp_idx = 29;
	debug_cbuf(alarm_buf[0], &tmp_idx, false);
}

void readConsole() {

	int serialStrIdx;
	int inByte = 0;
	char serialIn[50];
	const int termChar = 13; //Terminate lines with CR

	memset(serialIn,0,sizeof(serialIn));

	inByte = Serial.read();

        if (inByte < 0) return;

//Serial.print("I received: ");
//Serial.println(inByte, DEC);
	serialStrIdx=0;



	//If we see data (inByte > 0) and that data isn't a carriage return

	if (inByte > 0 && inByte != termChar) {
		do {
			if ( inByte == termChar ) break;

			serialIn[serialStrIdx] = inByte; // Save the data in a character array
			serialStrIdx++; //Increment position in array
			inByte = Serial.read(); // Read next byte
		} while (serialStrIdx < 50);

		serialIn[serialStrIdx] = 0; //Null terminate the serialIn
	}

Serial.println(serialIn);
	//run commands
	if (strncmp( serialIn, "debug", 5) == 0 ) {
		on_debug();
	}
}

void loop()
{

	if (!vista.available()) {
		if (lastgidx != gidx) {
			debug_cbuf(gcbuf, &gidx, false);
			lastgidx = gidx;
//		      Serial.println(lastgidx);
//		      Serial.println(gidx);
		}
                readConsole();

		return;
	}

	char x;

    x = vista.read();
    if (!x)  {
//      Serial.print(".");
      return;
    }

	//start of new message
	if ((int)x == 0xFFFFFFF7) {
		guibuf[ guidx ] = x;
		guidx++;

		read_chars( msg_len_status -1, guibuf, &guidx, 100);

		on_display(guibuf, &guidx);
		return;
	}

	//key ack
	if ((int)x == 0xFFFFFFF6) {
		//debug_cbuf(gcbuf, &gidx, true);

		gcbuf[ gidx ] = x;
		gidx++;

		read_chars( msg_len_ack -1, gcbuf, &gidx, 30);

		on_ack(gcbuf, &gidx);
		return;
	}

	//state chage?
	if ((int)x == 0xFFFFFFF2) {
		//debug_cbuf(gcbuf, &gidx, true);

		gcbuf[ gidx ] = x;
		gidx++;

		read_chars_dyn( gcbuf, &gidx, 30);

		on_status(gcbuf, &gidx);
		return;
	}


   gcbuf[ gidx ] = x;
   gidx++;

	if (gidx >= 30) {
		Serial.print("Unknown Buffer overflow: ");
		debug_cbuf(gcbuf, &gidx, true);
		return;
	}
}



void send_email() {

	Serial.println("SMTP opening connection...");

	EthernetClient client;
}


void setup()   {                
	// initialize the digital pin as an output:
	pinMode(ledPin, OUTPUT);     
	blink_alive();

	//initialize USB serial
	Serial.begin(115200);
	Serial.println("good morning");

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
	#endif

	#ifdef USE_SMTP
//		send_email();
	#endif

	#ifdef USE_HTTP
//		api_call_register_self();
	#endif

	#ifdef USE_ZMQ
//		api_call_register_self();
	#endif

	Serial.println("Starting vista keypad bus"); 
	vista.begin( vistaBaud );
}
