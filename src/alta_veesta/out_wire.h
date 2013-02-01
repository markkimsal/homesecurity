/**
 * Header file for writing out data to vista panel
 */

#ifndef out_wire_h
#define out_wire_h

#include "config.h"
#include "alta_veesta.h"
#include "SoftwareSerial2.h"

void write_chars(
		SoftwareSerial &vistaSerial,
	   	char cbuf[],
	   	int *idx,
	   	bool clear
);


void ask_for_write(
		SoftwareSerial &vistaSerial
);

#endif
