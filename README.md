# ESP_E58-Drone
Controlling an E58 drone with an ESP uC 

What I have learned so far:

- The Eachine E58 drone uses the Lewei LW9809 camera/WiFi modul (http://www.le-wei.com/eacp_view.asp?id=66)
- It opens an access point and has a DHCP server, that can connect at least two WiFi clients at the same time
- An ESP8266 can connect to the drone without any problems (even in parallel to the JYUfo App)
- The E58 drone can be operated via WiFi and the JYUfo App without major problems via an intermediate ESP8266 router (https://github.com/martin-ger/esp_wifi_repeater)
- The Eachine E58 drone uses the WiFi protocol described here: https://blog.horner.tj/hacking-chinese-drones-for-fun-and-no-profit/
- The enclosed Arduino sketch starts the drone (CAUTION: no further control - keep it in your hand or it will crash!)
- Indedendent of the WiFi IP address of smartphone, the JYUfo App always wants to connect to IP 192.168.0.1
- As soon as the drone gets no more messages on UDP port 50000, the drone stops its motors immediately
- The control messages to UDP port 50000 are send about every 50ms
- The take off command (byte 5: 01) is sent 22 times in a row (about 1.1 s)
- The JYUfo App sends, as soon as the contol interface is on, about once per second a UDP message to port 40000, content seems to be constant 7 bytes: 63 63 01 00 00 00 00

Ideas:
- There seems to be a one-way serial protocol from the camera/WiFi controller to the flight controller (1 wire, see https://www.youtube.com/watch?v=HoZUKzStchg 9:55). What about this protocol (same as on port 50000)? 
- Would it be possible to add a GPS controller via WiFi (or serial connection)?
- The non-intrusive idea for a GPS controller via WiFi would be, to use an intermediate ESP between the App and the drone (as router). This router could forward all move commands from the controller, thus you can fly the drone normally. As soon as the controller sends no more movements, the GPS takes over an tries to hold the position. Orientation of the drone could either be deduced from the last movements or with an additional compass.
