
#include "api_call_zmq.h"


#include <SPI.h>
#include <Ethernet.h>

extern prog_char msg_server_path[];
extern prog_char msg_server_host[];
extern prog_uint16_t msg_server_port;

void api_call_zmq_register_self(byte amsg_server[]) {

	Serial.println("ZMQ opening connection...");

	unsigned int port = pgm_read_word( msg_server_port );

	char path[ 30 ];
	char host[ 20 ];
	strcpy_P(path, msg_server_path);
	strcpy_P(host, msg_server_host);


	/*
	EthernetClient client;


	//if (!client.connect(amsg_server, port)) {
	if (!client.connect("192.168.2.77", port)) {
		Serial.println("HTTP failed");
		return;
	}

	char d;

	Serial.println("HTTP connected");
	client.print("GET ");
	client.print(path);
	client.print(" HTTP/1.0");
	client.println();
	client.print("Connection:close");
	client.println();
	client.println();
	delay(200);
	while (d = client.read()) {
		Serial.print(d);
	}
	Serial.println();

	client.stop();
	*/
}

void api_call_zmq_alarm(byte amsg_server[]) {

	Serial.println("ZMQ opening connection...");

	unsigned int port = pgm_read_word( msg_server_port );

	char path[ 30 ];
	char host[ 20 ];
	strcpy_P(path, msg_server_path);
	strcpy_P(host, msg_server_host);
}
