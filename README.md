# ESP_E58-Drone
Controlling an E58 drone with an ESP8266 uC

The idea of this project is to use a really cheap drone as **plattform for experiements with GPS, compass, optical flow, and other sensors and finally to build an autonomous drone**. To achive that, first I had to find a way for simply adding new sensors and control software, if possible without reengineering the the whole onboard-software. The approach is, to add an additional ESP8266 controller onboard and let it interface with the drone's native controller via WiFi.

## The Drone

The Eachine E58 (https://www.eachine.com/de/EACHINE-E58-WIFI-FPV-With-2MP-Wide-Angle-Camera-High-Hold-Mode-Foldable-RC-Drone-Quadcopter-RTF-p-1045.html) is a small (< 100g) and very cheap foldable toy drone (< 30€) that looks pretty much like its bigger brothers from china. It has separate RC and WiFi(!) control, reasonable flight characteristics and even a camera with somewhat poor image quality. However it has neither GPS nor an optical flow sensor for stabilization. This has to be done manually and can be very tricky if there is wind outside.

The basic controller and the protocols seem to be quite similar to bunch of other chinese drones. They all use similar smartphone apps for video and control, which all seem to be variants from the same OEM build. So the results of this project should be portable to many similar drones.


## The Hardware Mod

To save weight I tried to use the drones battery also as power supply for the additional hardware. This seems to work at least for an ESP8266 and a compass modul. 

First I opened the drone, just 6 screws and you can remove the upper body part:
<img src="https://raw.githubusercontent.com/martin-ger/ESP_E58-Drone/main/IMG_20201025_112509899_HDR_s.jpg">

Now I soldered 2 wires to the down side of the battery/switch PCB - just plain B+ and B-:
<img src="https://raw.githubusercontent.com/martin-ger/ESP_E58-Drone/main/IMG_20201025_113207185_HDR_s.jpg">

Made a small hole in the body behind the left front arm to get the wire out:
<img src="https://raw.githubusercontent.com/martin-ger/ESP_E58-Drone/main/IMG_20201025_121400787_HDR_s.jpg">

Mounted a Wemos D1 mini on the back of the drone and connected battery power via a small switch to 5V and GND - in the front you see a GY-511 LSM303DLHC compass modul connected via I2C to the ESP:
<img src="https://raw.githubusercontent.com/martin-ger/ESP_E58-Drone/main/IMG_20201025_151930113_HDR_s.jpg">

This is now my platform for further experiments...

## Results
The drone can fly via the App as normal, while it is finally controlled by the intermediate ESP8266. The E58Contol.ino sketch will connect to the drone (change STASSID for your drone) and offers a WiFi SSID "E58drone". When you connect your JYUfo App to this SSID "E58drone", you can fly the drone as normal. When the App turns off (or loses signal to the ESP's WiFi network) the ESP will continue to send control commands to the drone. Currently it just tells it to keep on hovering, but here a more intelligent controll might come in (GPS).

## What I have learned so far:

- The drone can lift at least an GPS-Modul, its Antenna, and a WEMOS D1 mini (22g)
- The battery is powerful enough to supply the drone and the additional ESP8266 (around 70mA plus)
- The GY-511 LSM303DLHC compass modul seems to work on the drone
- Tests with the data of an ublox NEO-6M GPS receiver and http://freenmea.net/ were successful, best GPS accuracy with 10 satellites is less than 1m 
- The Eachine E58 drone itself has a Lewei LW9809 camera/WiFi modul (http://www.le-wei.com/eacp_view.asp?id=66)
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
- The JYUfo App sends, as soon as the contol interface is on a UDP message to port 40000, content seems to be these 7 bytes: 63 63 XX 00 00 00 00, where XX is 01 or 02 (first connect/re-connect?). No real idea about its meaning.
- If there is no drone response, it repeats this message to port 40000 about once per second.

## Further Ideas:
- There seems to be a one-way serial protocol from the camera/WiFi controller to the flight controller (1 wire, see https://www.youtube.com/watch?v=HoZUKzStchg 9:55). What about this protocol (same as on port 50000)? 
- Would it be possible to add a GPS controller via WiFi (or serial connection)?
- The non-intrusive idea for a GPS controller via WiFi would be, to use an intermediate ESP between the App and the drone (as router). This router could forward all move commands from the controller, thus you can fly the drone normally. As soon as the controller sends no more movements, the GPS takes over an tries to hold the position. Orientation of the drone could either be deduced from the last movements or with an additional compass.

## Other Programs:
- The E58.ino sketch simply starts the drone by sending the UDP command to port 50000 (CAUTION: no further control - keep it in your hand or it will crash!)
