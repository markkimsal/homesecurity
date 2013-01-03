# VISTA ICM Replacement
This project is designed to allow you to connect an Arduino-like device to your Honeywell (Ademco) security panel and "listen in" for key events.  This project is an implemenation of reverse engineering the Ademco ECP keypad bus.

#Web Notifications
You can configure this project to ping a web server with any message you want when an alarm occurs.  This allows you to use a more powerful server to give you access to more powerful communication options (SMS, Email over SSL/STARTTLS, HTTPS, etc.)

#Config
There are a few configurations available.  Most simply print CSV (Excel) compatible debugging of each signal.  Signals are decoded in ASCII, decimal, and hexidecimal values and printed to the serial port as comma separated (the last item has an extra comma after it, it's not missing data)

You can also setup your web server configuration in the config.h file.  The processing power on the Arduino is limited so you cannot do SSL or even e-mail (because most gateways require TLS wrapped SMTP connections).  Sending a small packet to a web server allows you to extend the capabilities of the Arduino with a more powerful CPU.

#Hardware Setup
If you're in the U.S you can get everything you need from Radio Shack.  Pick up an Arduino, an Ethernet Shield, a Proto Shield (or a breadboard), 2 5v regulators, some resistors, some wire, some soldering equipment.  Power the Arduino device from the keypad 12 volt wire by putting the keypad 12 volt wire into the 5v regulator and connect the output to the Arduino 5 volt in pin.  Connect the grounds.  Connect the data-out wire (yellow) to another 5 volt regulator and put the output of this regulator to pin 8 on the Arduino.  Connect the data in wire (green) to pin 7.  Ground the Arduino to the keypad ground wire (black).

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
