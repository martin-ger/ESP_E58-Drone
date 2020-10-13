/*
  Very basic code to start the Eachine E58 drone
  CAUTION: it will start - no control afterwards!!
*/


#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#ifndef STASSID
#define STASSID "WiFi-720P-2CDC1C"
#define STAPSK  ""
#endif

unsigned int localPort = 8888;      // local port to listen on

WiFiUDP Udp;

byte msg[] = {0x66, 0x80, 0x80, 0x80, 0x80, 0x01, 0x00, 0x99, 0x0};
IPAddress ip(192, 168, 0, 1);
int count = 100;

void setup() {
  Serial.begin(115200);
  Serial.println();
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  Serial.printf("UDP client on port %d\n", localPort);
  Udp.begin(localPort);
}

void loop() {
  if (count > 0) {
    // send a reply, to the IP address and port that sent us the packet we received
    Udp.beginPacket(ip, 50000);
    msg[6] = msg[1] ^ msg[2] ^ msg[3] ^ msg[4] ^ msg[5];
    Udp.write((char*)msg);
    Udp.endPacket();
    Serial.println("send.");
    count--;
  }
  delay(50);
}
