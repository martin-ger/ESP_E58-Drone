# ESP_E58-Drone
Controlling an E58 drone with an ESP uC 

What I have learned so far:

- The Eachine E58 drone uses the WiFi protocol described here: https://blog.horner.tj/hacking-chinese-drones-for-fun-and-no-profit/
- It uses the Lewei LW9809 camera/WiFi modul (http://www.le-wei.com/eacp_view.asp?id=66)
- It opens an access point and has a DHCP server, that can connect at least two WiFi clients at the same time
- An ESP8266 can connect to the drone without any problems (even in parallel to the JY Ufo APP)
- The enclosed Arduino sketch starts the drone (CAUTION: no further control - keep it in your hand or it will crash!)
- As soon as it gets no more messages on UDP port 50000, the drone stops its motors immediately

Ideas:
- There seems to be a one-way serial protocol from the camera/WiFi controller to the flight controller (1 wire, see https://www.youtube.com/watch?v=HoZUKzStchg 9:55). What about this protocol (same as on port 50000)? 
- Would it be possible to add a GPS controller via WiFi (or serial connection)?
