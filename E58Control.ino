
/*
  Very basic code to control the Eachine E58 drone
*/

#if !LWIP_FEATURES || LWIP_IPV6
#error "NAT routing not available"
#endif

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <lwip/napt.h>
#include <lwip/dns.h>
#include <lwip/netif.h>
#include <netif/etharp.h>
#include <lwip/udp.h>
#include <dhcpserver.h>

#ifndef STASSID
#define STASSID "WiFi-720P-2CDC1C"
#define STAPSK  ""
#define APSSID "E58drone"
#endif

#define ControlPort 50000
#define LocalPort   8888
#define NAPT 1000
#define NAPT_PORT 10

// Command Bits sent to the drone

#define  CMD_NULL       0x00
#define  CMD_TAKE_OFF   0x01
#define  CMD_LAND       0x02
#define  CMD_EMERGENCY  0x04
#define  CMD_ROLL       0x08
#define  CMD_HEADLESS   0x10
#define  CMD_LOCK       0x20
#define  CMD_UNLOCK     0x40
#define  CMD_CALIBRATE  0x80

PACK_STRUCT_BEGIN
struct tcp_hdr {
  PACK_STRUCT_FIELD(u16_t src);
  PACK_STRUCT_FIELD(u16_t dest);
  PACK_STRUCT_FIELD(u32_t seqno);
  PACK_STRUCT_FIELD(u32_t ackno);
  PACK_STRUCT_FIELD(u16_t _hdrlen_rsvd_flags);
  PACK_STRUCT_FIELD(u16_t wnd);
  PACK_STRUCT_FIELD(u16_t chksum);
  PACK_STRUCT_FIELD(u16_t urgp);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END

WiFiUDP Udp;
unsigned long last_controller_msg_time;

// Fixed address of the drone
IPAddress ip(192, 168, 0, 1);

static netif_input_fn orig_input_drone;
static netif_linkoutput_fn orig_output_drone;

bool check_packet_in(struct pbuf *p) {
  return true;
}

err_t my_input_drone (struct pbuf *p, struct netif *inp) {

  if (check_packet_in(p)) {
    return orig_input_drone(p, inp);
  } else {
    pbuf_free(p);
    return ERR_OK;
  }
}

bool check_packet_out(struct pbuf *p) {
struct eth_hdr *mac_h;
struct ip_hdr *ip_h;
struct udp_hdr *udp_he;
struct tcp_hdr *tcp_h;
char *payload;

  if (p->len < sizeof(struct eth_hdr))
    return false;

  mac_h = (struct eth_hdr *)p->payload;

  // Check only IPv4 traffic
  if (ntohs(mac_h->type) != ETHTYPE_IP)
    return true;

  if (p->len < sizeof(struct eth_hdr)+sizeof(struct ip_hdr))
    return false;

  ip_h = (struct ip_hdr *)(p->payload + sizeof(struct eth_hdr));

  if (IPH_PROTO(ip_h) == IP_PROTO_UDP) {
    if (p->len < sizeof(struct eth_hdr)+sizeof(struct ip_hdr)+sizeof(struct udp_hdr))
      return false;

    udp_he = (struct udp_hdr *)(p->payload + sizeof(struct eth_hdr) + sizeof(struct ip_hdr));
    payload = (char*)(p->payload + sizeof(struct eth_hdr) + sizeof(struct ip_hdr) + sizeof(struct udp_hdr));

    if (ntohs(udp_he->dest) == ControlPort && ntohs(udp_he->src) != LocalPort) {
      last_controller_msg_time = millis();
      Serial.printf("X %+04d Y %+04d Z %+04d R %+04d C %x\n", payload[1]-0x80, payload[2]-0x80, payload[3]-0x80, payload[4]-0x80, payload[5]); 
    }
  }
/*
  else if (IPH_PROTO(ip_h) == IP_PROTO_TCP) {
    if (p->len < sizeof(struct eth_hdr)+sizeof(struct ip_hdr)+sizeof(struct tcp_hdr))
      return false;

    tcp_h = (struct tcp_hdr *)(p->payload + sizeof(struct eth_hdr) + sizeof(struct ip_hdr));
  }
*/
  return true;
}

err_t my_output_drone (struct netif *outp, struct pbuf *p) {
  if (check_packet_out(p)) {
    return orig_output_drone(outp, p);
  } else {
    pbuf_free(p);
    return ERR_OK;
  }
}

// patches the netif to insert the filter functions
void patch_netif(ip_addr_t netif_ip, netif_input_fn ifn, netif_input_fn *orig_ifn, netif_linkoutput_fn ofn, netif_linkoutput_fn *orig_ofn)
{
struct netif *nif;

  for (nif = netif_list; nif != NULL && nif->ip_addr.addr != netif_ip.addr; nif = nif->next);
  if (nif == NULL) return;

  if (ifn != NULL && nif->input != ifn) {
    *orig_ifn = nif->input;
    nif->input = ifn;
  }
  if (ofn != NULL && nif->linkoutput != ofn) {
    *orig_ofn = nif->linkoutput;
    nif->linkoutput = ofn;
  }
}

void send_control_message(byte x, byte y, byte z, byte rot, byte command){
  static byte msg[] = {0x66, x, y, z, rot, command, 0x00, 0x99};
  msg[6] = msg[1] ^ msg[2] ^ msg[3] ^ msg[4] ^ msg[5];
  
  Udp.beginPacket(ip, 50000);
  Udp.write(msg, sizeof(msg));
  Udp.endPacket();
}

void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nEPS E58 Drone Controller\n");

  // first, connect to STA so we can get a proper local DNS server
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, "");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  
  Serial.printf("\nSTA: %s\n", WiFi.localIP().toString().c_str());

  WiFi.softAPConfig(  // enable AP, with android-compatible google domain
    IPAddress(172, 217, 28, 254),
    IPAddress(172, 217, 28, 254),
    IPAddress(255, 255, 255, 0));
  WiFi.softAP(APSSID, "");
  Serial.printf("AP: %s\n", WiFi.softAPIP().toString().c_str());

  err_t ret = ip_napt_init(NAPT, NAPT_PORT);

  if (ret == ERR_OK) {
    ret = ip_napt_enable_no(SOFTAP_IF, 1);
    if (ret == ERR_OK) {
      Serial.printf("Initialization successful, connect to '%s'\n",APSSID);
    }
  }

  if (ret != ERR_OK) {
    Serial.printf("Initialization failed\n");
  }
  
  // Insert the filter functions
  patch_netif(WiFi.localIP(), my_input_drone, &orig_input_drone, my_output_drone, &orig_output_drone);

  // Set local port
  Udp.begin(LocalPort);

  last_controller_msg_time = millis();
}

void loop() {
/*  for (int i = 1; i < 200; i++) {
    send_control_message(0x80, 0x80, 0x01, 0x80, 0x40); 
    delay(50);
  }
*/
  //Serial.printf("Last Controller Msg: %d\n", millis() - last_controller_msg_time);
  if (millis() - last_controller_msg_time  > 170) {
    // send your own control message - "Stay as you are" at the moment
    send_control_message(0x80, 0x80, 0x80, 0x80, CMD_NULL);
  }
  delay(50);
}
