# ESP32_remote_longterm_logging
Automatic long-term logging for remote ESP32 IoT devices, by the Web Browser or an CURL script

read time: 30 min / understand time: 1 hour / test-it-out time: 1 hours

## This project will tell you a story you may already know well: after many days of developing an IoT device and spending also some nights to test it in your workroom, you finally install it in the target location. <br/>Then, after many days of bugless work, you surprisingly encounter some wrong results in the IoT data. <br/>Such errors can be seldom and hard to debug, out there in the real world environment ... so you need a remote logger for your ESP32!

For me, it was a Solar-Power Meter, built with ESP32 and INA219 and WiFi, installed under the roof, to measure the Solar current/voltage and sending it to ThinSpeak cloud. 
Weeks later, the data visible in the ThingSpeak cloud suddenly showed a single measure of maximum solar power at midnight. That's impossible in Germany for sure, but what is the root cause for it? Is it something in the Solar Controller, some strange overvoltage, or a bug in my ESP32 IoT device? It happened circa twice a month, so and I wanted to fix it.

Known issue: the Arduino OTA feature can only send code to the remote ESP32, but does not connect the serial port over WiFi
There have been a couple of ideas to receive log entries from a remote ESP32 IoT device connected to WiFi on your Laptop or PC, just to mention two: 
- implement Telnet on ESP32 and use PUTTY locally on your Laptop/PC to retrieve the serial log from the ESP,
- other solutions suggest to redirect the TX pin output of the standard Serial.Print() to a buffer which could be requested by a Web Browser,

