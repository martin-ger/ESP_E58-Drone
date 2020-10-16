# ESP_E58-Drone
Controlling an E58 drone with an ESP uC 

What I have learned so far:

- The drone can easily lift at least one additional battery pack (18g)
- The Eachine E58 drone uses the Lewei LW9809 camera/WiFi modul (http://www.le-wei.com/eacp_view.asp?id=66)
- It opens an access point and has a DHCP server, that can connect at least two WiFi clients at the same time
- An ESP8266 can connect to the drone without any problems (even in parallel to the JYUfo App)
- The E58 drone can be operated via WiFi and the JYUfo App without major problems via an intermediate ESP8266 router (https://github.com/martin-ger/esp_wifi_repeater)
- The Eachine E58 drone uses the WiFi protocol described here: https://blog.horner.tj/hacking-chinese-drones-for-fun-and-no-profit/
```
    1st byte – Header: 66
    2nd byte – Left/right movement (0-254, with 128 being neutral)
    3rd byte – Forward/backward movement (0-254, with 128 being neutral)
    4th byte – Throttle (elevation) (0-254, with 128 being neutral)
    5th byte – Turning movement (0-254, with 128 being neutral)
    6th byte – Reserved for commands (0 = no command)
    7th byte – Checksum (XOR of bytes 2, 3, 4, and 5)
    8th byte – Footer: 99

Command Bits (can be ORed):
    01 – Auto Take-Off
    02 - Land
    04 - Emergency Stop
    08 - 360 Deg Roll
    10 - Headless mode
    20 - Lock
    40 – Unlock Motor
    80 – Calibrate Gyro
 ```
 
- Indedendent of the WiFi IP address of smartphone, the JYUfo App always wants to connect to IP 192.168.0.1
- As soon as the drone gets no more messages on UDP port 50000, the drone stops its motors immediately
- The control messages to UDP port 50000 are send about every 50ms
- The take off command (byte 5: 01) is sent 22 times in a row (about 1.1 s)
- The JYUfo App sends, as soon as the contol interface is on, about once per second a UDP message to port 40000, content seems to be constant 7 bytes: 63 63 01 00 00 00 00

Ideas:
- There seems to be a one-way serial protocol from the camera/WiFi controller to the flight controller (1 wire, see https://www.youtube.com/watch?v=HoZUKzStchg 9:55). What about this protocol (same as on port 50000)? 
- Would it be possible to add a GPS controller via WiFi (or serial connection)?
- The non-intrusive idea for a GPS controller via WiFi would be, to use an intermediate ESP between the App and the drone (as router). This router could forward all move commands from the controller, thus you can fly the drone normally. As soon as the controller sends no more movements, the GPS takes over an tries to hold the position. Orientation of the drone could either be deduced from the last movements or with an additional compass. 

Programs:
- The E58.ino sketch simply starts the drone by sending the UDP command to port 50000 (CAUTION: no further control - keep it in your hand or it will crash!)

- The E58Contol.ino sketch will connect to the drone (change STASSID for your drone) and offers a WiFi network "E58drone". When you connect your JYUfo App to this network, you can fly the drone. When the App turns off (or loses signal to the ESP's WiFi network) the ESP will continue to send control commands to the drone. Currently it just tells it to keep on hovering, but here a more intelligent controll might come in (GPS).
