
#include "api_call_http.h"

#include <SPI.h>
#include <Ethernet.h>

extern prog_uint16_t msg_server_port;
extern prog_char msg_server_host[];
extern prog_char msg_server_path[];

void api_call_http_register_self(byte amsg_server[]) {


	char path[ 30 ];
	char host[ 30 ];
	strcpy_P(path, msg_server_path);
	strcpy_P(host, msg_server_host);

	EthernetClient client;


	Serial.println("HTTP opening connection...");
	/*
	if (!ether.dnsLookup(website))
		  Serial.println("DNS failed");
	else
		  Serial.println("DNS resolution done");
	ether.printIp("SRV IP:\t", ether.hisip);
	*/

	if (!client.connect(amsg_server, pgm_read_word(&msg_server_port))) {
		Serial.println("HTTP failed");
		return;
	}

	char d;
	String netbuf = "GET ";
    netbuf += path;
	netbuf += " HTTP/1.0\nHost: " + host + "\nConnection: close\n\n\n";

	Serial.println("HTTP connected");

	char sockbuf[ netbuf.length() ];
	netbuf.toCharArray(sockbuf, netbuf.length());
	client.write(sockbuf);
	delay(100);
	while(client.connected()) {
		while(client.available())   {
			d = client.read();
			if (d == '\n')
				Serial.print("\r\n");
			else
				Serial.print(d);
		}
	}	  
	client.stop();
	Serial.println();
}
