# Arduino + Vista Home Security
This project is designed to allow you to connect an arduino-like device to your Honeywell (Ademco) security panel and "listen in" for key events.

#Web Notifications
You can configure this project to ping a web server with any message you want when an alarm occurs.  This allows you to use a more powerful server to give you access to more powerful communication options (SMS, Email over SSL/STARTTLS, HTTPS, etc.)

#Config
There are a few configurations available.  Most simply print CSV (Excel) compatible debugging of each signal.  Signals are decoded in ASCII, decimal, and hexidecimal values and printed to the serial port as comma separated (the last item has an extra comma after it, it's not missing data)

#Hardware Setup
If you're in the U.S you can get everything you need from Radio Shack.  Pick up an Arduino, an Ethernet Shield, a Proto Shield (or a breadboard), 2 5v regulators, some resistors, some wire, some soldering equipment.  Power the Arduino device from the keypad 12 volt wire by putting the keypad 12 volt wire into the 5v regulator and connect the output to the Arduino 5 volt in pin.  Connect the grounds.  Connect the data-out wire (yellow) to another 5 volt regulator and put the output of this regulator to pin 8 on the arduino.  Connect the data in wire (green) to pin 7.  Ground the arduino to the keypad ground wire (black).

#License
This project users some parts of Arduino IDE - specifically the SoftwareSerial library.  So, whatever license that is under, this project is under (for the time being).
