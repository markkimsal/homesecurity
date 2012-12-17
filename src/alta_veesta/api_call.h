
/**
 * Header file for calling API methods on a server
 */

#ifndef api_call_h
#define api_call_h

#include "alta_veesta.h"

void api_call_register_self();

void api_call_alarm();

void api_call_init();

#ifdef USE_HTTP
#include "api_call_http.h"
#endif

#ifdef USE_ZMQ
#include "api_call_zmq.h"
#endif


#endif
