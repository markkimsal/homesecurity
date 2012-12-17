/**
 * Header file for calling API methods on a server
 */

#ifndef api_call_zmq_h
#define api_call_zmq_h

#include "alta_veesta.h"

void api_call_zmq_register_self(byte amsg_server[]);
void api_call_zmq_alarm(byte amsg_server[]);
void api_call_zmq_init(byte amsg_server[]);

#endif
