#
#
#atmega328.name=Arduino Duemilanove w/ ATmega328

#atmega328.upload.protocol=arduino
#atmega328.upload.maximum_size=30720
#atmega328.upload.speed=57600

#atmega328.bootloader.low_fuses=0xFF
#atmega328.bootloader.high_fuses=0xDA
#atmega328.bootloader.extended_fuses=0x05
#atmega328.bootloader.path=atmega
#atmega328.bootloader.file=ATmegaBOOT_168_atmega328.hex
#atmega328.bootloader.unlock_bits=0x3F
#atmega328.bootloader.lock_bits=0x0F

#atmega328.build.mcu=atmega328p
#atmega328.build.f_cpu=16000000L
#atmega328.build.core=arduino
#atmega328.build.variant=standard


ARDUINO_DIR = /home/mark/Programs/Arduino/arduino-1.0

BOARD_TAG    = atmega328
ARDUINO_PORT = /dev/ttyUSB*

ARDUINO_LIBS=SPI Ethernet Ethernet/utility
#PROJ_LIBS=./SoftwareSerial2

#ARDUINO_LIBS = Ethernet Ethernet/utility SPI


AVR_TOOLS_PATH   = /usr/bin
AVRDUDE_CONF     = /etc/avrdude.conf


# Which variant ? This affects the include path
VARIANT = 'standard'

# processor stuff
MCU   = 'atmega328p'

F_CPU = 16000000L

# normal programming info
AVRDUDE_ARD_PROGRAMMER = arduino
AVRDUDE_ARD_BAUDRATE   = 57600

# fuses if you're using e.g. ISP
ISP_LOCK_FUSE_PRE  = '0x3F'

ISP_LOCK_FUSE_POST = '0x0F'

#atmega328.bootloader.low_fuses=0xFF
#atmega328.bootloader.high_fuses=0xDA
#atmega328.bootloader.extended_fuses=0x05

ISP_HIGH_FUSE      = '0xDA'

ISP_LOW_FUSE       = '0xFF'

ISP_EXT_FUSE       = '0x05'



#Include your arduino-mk project here
include /home/user/Programs/Arduino/arduino-mk-0.8/Arduino.mk
