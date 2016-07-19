
#ifdef HAVE_NETWORK
#include <avr/pgmspace.h>
#include "api_call.h"
#include <SPI.h>
#include <Ethernet.h>

const char  msg_server[]         PROGMEM  = API_CALL_SERV; 
const char  msg_server_host[]    PROGMEM  = API_CALL_HOST; 
const char  msg_server_path[]    PROGMEM  = API_CALL_PATH;
const uint16_t  msg_server_port  PROGMEM  = API_CALL_PORT;

void api_call_register_self() {

	byte  amsg_server[4];
	amsg_server[0] = pgm_read_byte(&msg_server[0]);
	amsg_server[1] = pgm_read_byte(&msg_server[1]);
	amsg_server[2] = pgm_read_byte(&msg_server[2]);
	amsg_server[3] = pgm_read_byte(&msg_server[3]);


	#ifdef USE_HTTP
		api_call_http_register_self(amsg_server);
	#endif

	#ifdef USE_ZMQ
		//Serial.println("ZMQ opening connection...");
		api_call_zmq_register_self(amsg_server);
	#endif
}

void api_call_alarm() {

	byte  amsg_server[4];
	amsg_server[0] = pgm_read_byte(&msg_server[0]);
	amsg_server[1] = pgm_read_byte(&msg_server[1]);
	amsg_server[2] = pgm_read_byte(&msg_server[2]);
	amsg_server[3] = pgm_read_byte(&msg_server[3]);


	#ifdef USE_HTTP
		api_call_http_alarm(amsg_server);
	#endif

	#ifdef USE_ZMQ
		//Serial.println("ZMQ opening connection...");
		api_call_zmq_alarm(amsg_server);
	#endif
}

void api_call_init() {

	byte  amsg_server[4];
	amsg_server[0] = pgm_read_byte(&msg_server[0]);
	amsg_server[1] = pgm_read_byte(&msg_server[1]);
	amsg_server[2] = pgm_read_byte(&msg_server[2]);
	amsg_server[3] = pgm_read_byte(&msg_server[3]);


	#ifdef USE_HTTP
		api_call_http_init(amsg_server);
	#endif

	#ifdef USE_ZMQ
		//Serial.println("ZMQ opening connection...");
		api_call_zmq_init(amsg_server);
	#endif
}
#endif
