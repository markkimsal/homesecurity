#ifndef config_h
#define config_h

//#define KPADDR 18
const char VERSION[6] = "0.9.9";

#define DEBUG 0
#define DEBUG_STATUS  1
//#define DEBUG_KEYS    0
//#define DEBUG_DISPLAY 0


#define RX_PIN 12
#define TX_PIN 11

//try to enable network and DHCP?
//#define HAVE_NETWORK 1
//#define USE_DHCP     1
//#define STATIC_IP  {192, 168, 2, 104}

#define MAC_ADDR { 0xB8, 0x70, 0xF4, 0x1E, 0xC5, 0x7E };

// HTTP method is required for self-registering
#define USE_HTTP 1
//#define USE_SMTP 1
//#define USE_ZMQ  1


// Use LCD Screen
#define HAVE_OLED 1
#define OLED_SSD1306 1

//========================================
// Requires a custom web server
//========================================
#define API_CALL_HOST "example.com"
#define API_CALL_SERV {0, 0, 0, 0}
#define API_CALL_PATH "/homesecurity/api"
#define API_CALL_PORT 80

//========================================
// SMTP is not well supported
//========================================
//
/* IP Address of email server */
// Uverse
//byte email_server[] = { 98, 138, 84, 52 }; 
//int email_port = 465;

// ALT uverse
//byte email_server[] = { 68, 142, 198, 51 }; 

// yahoo mobile
//byte email_server[] = { 98, 136, 86, 109 };
//int email_port = 587;

#endif
