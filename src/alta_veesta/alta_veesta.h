#ifndef __ALTAVEESTA_H
#define __ALTAVEESTA_H

#include "Arduino.h"
#include <avr/pgmspace.h>
#include "config.h"
#include "SoftwareSerial2.h"

void switch_first_byte(int x, SoftwareSerial vista);

void expect_response(char byt);
void clear_expect();
void on_response_error( void (*func)(void *data) );
void on_response_complete( void (*func)(void *data) );

#endif
