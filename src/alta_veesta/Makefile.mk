#Change this to the location of your arduino 1.0 install
ARDUINO_DIR = /home/user/Programs/Arduino/arduino-1.0

#find out this value with 'make show_boards'
#BOARD_TAG    = atmega328
#BOARD_TAG    = uno


ARDUINO_PORT = /dev/ttyUSB*

#add any custom libs you need.
ARDUINO_LIBS=SPI Ethernet Ethernet/utility

AVR_TOOLS_PATH   = /usr/bin
AVRDUDE_CONF     = /etc/avrdude.conf

#Include your arduino-mk project here
include /home/user/Programs/Arduino/arduino-mk-0.8/Arduino.mk
