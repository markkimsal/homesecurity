
#ifdef HAVE_NETWORK
#include "api_call_http.h"

#include <SPI.h>
#include <Ethernet.h>

extern const uint16_t msg_server_port;
extern const char msg_server_host[];
extern const char msg_server_path[];

void api_call_http_register_self(byte amsg_server[]) {


	char path[ 30 ];
	char host[ 30 ];
	strcpy_P(path, msg_server_path);
	strcpy_P(host, msg_server_host);

	EthernetClient client;


	Serial.print("HTTP opening connection to ");
	Serial.println(host);
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
	netbuf += " HTTP/1.0\nHost: ";
	netbuf += host;
	netbuf += "\nConnection: close\n\n\n";

	Serial.print("HTTP connected to ... ");

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

void api_call_http_alarm(byte amsg_server[]) {
	api_call_http(amsg_server, "event=alarm");
}

void api_call_http(byte amsg_server[], char body[]) {

	char path[ 30 ];
	char host[ 30 ];
	strcpy_P(path, msg_server_path);
	strcpy_P(host, msg_server_host);

	EthernetClient client;


	if (!client) {
		Serial.println("Ethernet client not available!");
		return;
	}

	Serial.print("HTTP opening connection to ");
	Serial.println(host);
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
	//TODO: add serial number, hash message, JSON
	String postbuf = body;
	String netbuf = "POST ";
	netbuf += path;
	netbuf += " HTTP/1.0\nHost: ";
	netbuf += host;
	netbuf += "\nContent-Type: application/x-www-form-urlencoded\nContent-length:";
	netbuf += postbuf.length();
	netbuf += "\nConnection: close\n\n";
	netbuf += postbuf;
  //compile error on arduino 1.6.2
  //it's just a memory optimization
	//postbuf = NULL;

	Serial.println("HTTP connected");
	Serial.print(netbuf);

	char sockbuf[ netbuf.length() +1];
	netbuf.toCharArray(sockbuf, netbuf.length()+1);
	client.write(sockbuf);
	delay(300);
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

void api_call_http_init(byte amsg_server[]) {
	api_call_http(amsg_server, "event=test");
}
#endif
