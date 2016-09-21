# VISTA ICM Replacement
This project is designed to allow you to connect an Arduino-like device to your Honeywell (Ademco) security panel and "listen in" for key events.  This project is an implemenation of reverse engineering the Ademco ECP keypad bus.

# Development on Debian/Ubuntu
Install gcc-avr and avr-libc packages

Run scripts/bootstrap.sh to download Arduino-Makefile project.  This will unzip into a folder called "Arduino-Makefile"

Copy Makefile-Linux.mk files to Makefile.

Edit the Makefile to match your settings for libraries, Arduino IDE location, and TTY port settings.

Use make show\_boards  and make show\_submenu to find the right values"

  1. PROJECT\_DIR       = /path where this checkout is
  2. BOARD\_TAG         = pro<F9>
  3. BOARD\_SUB         = 8MHzatmega328

To compile and upload to your arduino run:

```
make upload
```


# Web Notifications
You can configure this project to ping a web server with any message you want when an alarm occurs.  This allows you to use a more powerful server to give you access to more powerful communication options (SMS, Email over SSL/STARTTLS, HTTPS, etc.)

Please note that it may be illegal in your area to trasmit signal information from an alarm system to a centralized station without a license.

# Config
There are a few configurations available.  Most simply print CSV (Excel) compatible debugging of each signal.  Signals are decoded in ASCII, decimal, and hexidecimal values and printed to the serial port as comma separated (the last item has an extra comma after it, it's not missing data)

You can also setup your web server configuration in the config.h file.  The processing power on the Arduino is limited so you cannot do SSL or even e-mail (because most gateways require TLS wrapped SMTP connections).  Sending a small packet to a web server allows you to extend the capabilities of the Arduino with a more powerful CPU.

# Hardware Setup
There are 3 different hardware configurations.  One for each level of complexity of your project.<F9>

### Just for Testing
You can run the panel's yellow wire through a 5v regulator - LM7805 - to get the keypad signals down to 5v.  Use LD1117V33 to step down to 3.3 signals if you're using a 3.3v Arduino.  Use a 13v tolarant diode between the arduino and the green wire.  If your cable is short enough, 5v from the Arduino should be enough to signal to the panel.  If you're using 3.3v, then this setup can only listen and can't talk back.  (Maybe, try it).


### Arduino powered as a keypad.

If you want the Arduino to be powered from the keypad line, you can use NPN transistors to convert the signals to 5V or 3.3V.

Connect one transistor's collector to a stable 5V from the regulator.  Use a large value resistor between the yellow wire from the panel and the base of the transistor.  The value should reduce the voltage to 0.6v range.  Something like 600k I think.

Connect the raw red power line to the other transistor's collector, and use a small value resistor in between the base of this transitor and the Arduino signaling pin.

Use the linear regulator to power the arduino.

### Arduino with Raspberry Pi

If you want to run a Raspberry Pi in your secutiry panel, the current from the red wire cannot supply stable load to the RPi.   You must draw power from another wall wart or by piggy-backing off the panel's wall wart - which is usually 16v AC.  Use a AC-DC step down converter or DC-DC step down converter with rectifier diodes or recitfier IC.  Then power the Arduino from the RPi's GPIO.  If you use diode rectifiers to convert AC to DC for a DC-DC step down converter, make sure the diodes can handle the amerage drawn by the RPi.  Buy large ones that are meant for mains, don't use tiny ones that you got in a kit.

If you do this, the relative voltage of the signals will be vary different, you must use optocouplers between the Arduino and the panel's signals.

Use optocouplers in place of all transistors in the second setup, and use a high value pulldown resistor on the output side.  PC817C seem to work fine, as well as MC14504b.

(The RPi runs at 5v, but signals at 3.3v.  If you run a 5v arduino, you will need to level shift the UART signals to communicate with the RPi.)

#Protocol
Essentially, the data out wire uses 8-bit, even parity, 1 stop bit, inverted, NRZ +12 volt TTL signals.  But, the data out wire also acts somewhat like a clock wire sometimes.  

Regular messages are transmitted periodically and begin with either F7 or F2.  F7 messages are fixed length, F2 are dynamic length.  Both message types consist of a header and a body section.  The second by of F2 messages indicate the remaining bytes for said message.

When behaving as a clock wire, the panel will pull the line low for an extended period of time (&gt;10ms).  When this happens, any device that has data to send should pusle a response.  If the panel selects that device (in case there are multiple devices needing to send data) an F6 will be broadcast on the data out wire with the address of the device which should send data.  F6 messages behave like normal serial data.


##Pulsing
In order to avoid "open circuit" errors, every device periodically sends their device address to the panel.  This happens after F7 messages, and as a response to input querying from the panel.

Pulsing doesn't exactly match up with serial data.  Timing is handled by synchronizing to rising edges sent from the server.  This can be faked by sending a 0xFF byte because the start bit will raise the line for a short time, and all the 1s will bring the line low (because the data signals are inverted).  There doesn't seem to be any parity bits sent during this pusling phase.

When multiple devices ack, their pulses should coincide.  The result is that the last bit is ANDed together.

Address 16 - 11111110  (inverted on the wire looks like 00000001)
Address 21 - 11011111  (inverted on the wire looks like 00100000)
Together on the wire they look like a single byte of    00100001 0x21

In order to get perfect AND logic from multiple devices sending pulses at the same time, they must pulse a start bit and no data (0xFF) twice before sending their address bit mask.  When the line is pulled low for more than 12 ms, and a keypad has some information to transmit, this pulsing should synchronize with the rising edges from the panel.


    (high)---\___(10ms low)___/---\______/---\_______/---\_____  (Yellow data-out)
    
    __________________________/\_________/\__________/\_/\_____  (0xFF, 0xFF, 0xEF) (Green)

#License
This project uses some parts of Arduino IDE - specifically the SoftwareSerial library.  So, whatever license that is under, this project is under (for the time being).

#Credits
I could not have cracked the nut of Vista ECP protocol without knowing that it was possible by reading Miguel Sanchez's article in Circuit Cellar.  Although he did not process signals sent from the panel, he figured out the baud rate, stop bit configuration, and the fact that the signal is inversed (0 is high, 1 is low).

Other projects that were invaluable to me are:

* Arduino SUMP analyzer firmware:
  * [http://github.com/gillham/logic_analyzer]
* Open Logic Sniffer (desktop):
  * [http://www.lxtreme.nl/ols/]
