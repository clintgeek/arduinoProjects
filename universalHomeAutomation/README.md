# arduinoHomeAutomation
Using Arduinos as HomeAutomation nodes

This Arduino sketch runs in conjunction with an C++ interface hosted on a rasberryPi to communicate with HomeAssistant.

C++ interface: https://github.com/clintgeek/rf24NetworkInterface

It has several functions including:
- RGB LED strip controller
- DHT 11/22 sensor for temp/humidity
- TV power sensor

Hardware used includes:
- Arduino Nano
- rf24L01 wireless adapter (using rf24Network)
- DHT11 and DHT22 temperature and humidity sensors
- 5050 SMD RGB LED strips
- 12 key matrix keypad
