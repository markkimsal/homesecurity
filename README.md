# Arduino + Vista Home Security
This project is designed to allow you to connect an Arduino-like device to your Honeywell (Ademco) security panel and "listen in" for key events.

#Web Notifications
You can configure this project to ping a web server with any message you want when an alarm occurs.  This allows you to use a more powerful server to give you access to more powerful communication options (SMS, Email over SSL/STARTTLS, HTTPS, etc.)

#Config
There are a few configurations available.  Most simply print CSV (Excel) compatible debugging of each signal.  Signals are decoded in ASCII, decimal, and hexidecimal values and printed to the serial port as comma separated (the last item has an extra comma after it, it's not missing data)

You can also setup your web server configuration in the config.h file.  The processing power on the Arduino is limited so you cannot do SSL or even e-mail (because most gateways require TLS wrapped SMTP connections).  Sending a small packet to a web server allows you to extend the capabilities of the Arduino with a more powerful CPU.

#Hardware Setup
If you're in the U.S you can get everything you need from Radio Shack.  Pick up an Arduino, an Ethernet Shield, a Proto Shield (or a breadboard), 2 5v regulators, some resistors, some wire, some soldering equipment.  Power the Arduino device from the keypad 12 volt wire by putting the keypad 12 volt wire into the 5v regulator and connect the output to the Arduino 5 volt in pin.  Connect the grounds.  Connect the data-out wire (yellow) to another 5 volt regulator and put the output of this regulator to pin 8 on the Arduino.  Connect the data in wire (green) to pin 7.  Ground the Arduino to the keypad ground wire (black).

#License
This project users some parts of Arduino IDE - specifically the SoftwareSerial library.  So, whatever license that is under, this project is under (for the time being).
