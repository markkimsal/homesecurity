/**
 * 
 */

int vistaIn = 8;
int vistaOut = 3;
int msg_len_status = 45;
int msg_len_ack = 4;


char gcbuf[100];
int  gidx = 0;
int  lastgidx = 0;

char guibuf[100];
int  guidx = 0;

#include "api_call.h"


#include "alta_veesta.h"
#include "config.h"


#include "SoftwareSerial2.h"
SoftwareSerial vista(vistaIn, vistaOut, true);

#include <SPI.h>
#include <Ethernet.h>

byte mac[] = { 0xF6, 0x75, 0x92, 0x78, 0x67, 0x3F };


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
	x++;

	if (c & 0x04) {
		ct = 19;
	} else {
		ct = 13;
	}
	if (c & 0x08) {
		ct = 24;
	}

	if (c == 0x0B) {
		ct = 7;
	}

	if (c == 0x1F) {
		ct = 25;
	}

	while (x < ct) {
		if (vista.available()) {
			c = vista.read();
			if (!c) continue;
			if (idxval >= limit) {
				
				Serial.print("Buffer overflow: ");
//				Serial.println(idxval, DEC);
//				Serial.println(limit, DEC);
				*idx = idxval;
				return;
			} 
			buf[ idxval ] = c;
			idxval++;
			x++;
		} else {
//			debug_cbuf(buf, &idxval, false);
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

void on_status(char cbuf[], int *idx) {


	//F2 12 unknown message type
	if ( cbuf[1] == 0x12) {
		return;
	}

	#ifdef DEBUG_STATUS
	//DEBUG
	debug_cbuf(cbuf, idx, false);
	#endif

	//16th spot is 01 for disarmed, 02 for armed
	short armed = 0x02 & cbuf[15];

	//17th spot is confusing
	//short unknown = 0x02 & cbuf[16];

	//18th spot is for bypass
	short bypass = 0x02 & cbuf[17];

	//19th spot is for alarm types
	//1 is no alarm
	//2 is no alarm
	//4 is a alarm 
	//6 is a fault that does not cause an alarm
	short alarm = (cbuf[18] & 0x04) && !(cbuf[18] & 0x02);

	if (alarm == 1 && armed ) {
		on_alarm();
		Serial.print ("ALARM!");
	} else if (alarm ==1) {
		Serial.print ("alarm canceled");
	} else {
		Serial.print ("no alarm");
	}

	memset(cbuf, 0, sizeof(cbuf));
	*idx = 0;
}

void on_display(char cbuf[], int *idx) {


	//first 8 bytes are headers
	for (int x = 10; x < *idx -1; x++) {
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

	Serial.print("Ack kp: ");
	Serial.println(kpadr, DEC);


	#ifdef DEBUG_KEYS
	//DEBUG
	debug_cbuf(cbuf, idx, false);
	#endif

	//clear memroy
	memset(cbuf, 0, sizeof(cbuf));
	*idx = 0;

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
		debug_cbuf(gcbuf, &gidx, true);

		gcbuf[ gidx ] = x;
		gidx++;

		read_chars( msg_len_ack -1, gcbuf, &gidx, 100);

		on_ack(gcbuf, &gidx);
		return;
	}

	//state chage?
	if ((int)x == 0xFFFFFFF2) {
		debug_cbuf(gcbuf, &gidx, true);

		gcbuf[ gidx ] = x;
		gidx++;

		read_chars_dyn( gcbuf, &gidx, 100);

		on_status(gcbuf, &gidx);
		return;
	}


//   print_hex(x, 8);
   gcbuf[ gidx ] = x;
   gidx++;
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
  

/*
	Serial.println("Starting ethernet with DHCP...");
	blink_dhcp();
	//ethernet
	if (Ethernet.begin(mac) == 0) {
		Serial.println("Failed to configure Ethernet using DHCP");
		// no point in carrying on, so do nothing forevermore:
		for(;;)
		;
	}
	// print your local IP address:
	Serial.println(Ethernet.localIP()); 
*/

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
