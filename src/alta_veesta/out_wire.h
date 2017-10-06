/**
 * Header file for writing out data to vista panel
 */

#ifndef out_wire_h
#define out_wire_h

#include "config.h"
#include "alta_veesta.h"
#include "SoftwareSerial2.h"

void write_chars(
		SoftwareSerial &vistaSerial
);


void ask_for_write(
		SoftwareSerial &vistaSerial
);

void out_wire_queue(
		char byte
);

void debug_out_buf();

bool have_message();

void key_ack_complete( void *data );
void out_wire_init();
#endif
