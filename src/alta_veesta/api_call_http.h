/**
 * Header file for calling API methods on a server
 */

#ifndef api_call_http_h
#define api_call_http_h

#include "alta_veesta.h"

void api_call_http_register_self(byte amsg_server[]);
void api_call_http_alarm(byte amsg_server[]);
void api_call_http_init(byte amsg_server[]);
void api_call_http(byte amsg_server[], char body[]);

#endif
