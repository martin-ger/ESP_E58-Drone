
// NAPT example released to public domain

#if LWIP_FEATURES && !LWIP_IPV6

#ifndef STASSID
#define STASSID "mynetwork"
#define STAPSK  "mynetworkpassword"
#endif

#include <ESP8266WiFi.h>
#include <lwip/napt.h>
#include <lwip/dns.h>
#include <lwip/netif.h>
#include <netif/etharp.h>
#include <lwip/udp.h>
#include <dhcpserver.h>

#define NAPT 1000
#define NAPT_PORT 10

IPAddress myIP;

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

static netif_input_fn orig_input_drone;
static netif_linkoutput_fn orig_output_drone;

bool check_packet_in(struct pbuf *p) {
struct eth_hdr *mac_h;
struct ip_hdr *ip_h;
struct udp_hdr *udp_he;
struct tcp_hdr *tcp_h;

  if (p->len < sizeof(struct eth_hdr))
    return false;

  mac_h = (struct eth_hdr *)p->payload;

  // Check only IPv4 traffic
  if (ntohs(mac_h->type) != ETHTYPE_IP)
    return true;

  if (p->len < sizeof(struct eth_hdr)+sizeof(struct ip_hdr))
    return false;

  ip_h = (struct ip_hdr *)(p->payload + sizeof(struct eth_hdr));
/*
  // Known MACs can pass
  for(int i = 0; i<max_client; i++) {
    if (memcmp(mac_h->src.addr, allowed_macs[i].addr, sizeof(mac_h->src.addr)) == 0) {
      return true;
    }
  }
*/

  // DHCP and DNS is okay
  if (IPH_PROTO(ip_h) == IP_PROTO_UDP) {
    if (p->len < sizeof(struct eth_hdr)+sizeof(struct ip_hdr)+sizeof(struct udp_hdr))
      return false;

    udp_he = (struct udp_hdr *)(p->payload + sizeof(struct eth_hdr) + sizeof(struct ip_hdr));
/*
    if (ntohs(udp_he->dest) == DHCP_PORT)
      return true;

    if (ntohs(udp_he->dest) == DNS_PORT)
      return true;

    return false;
*/
  }

  // HTTP is redirected
  if (IPH_PROTO(ip_h) == IP_PROTO_TCP) {
    if (p->len < sizeof(struct eth_hdr)+sizeof(struct ip_hdr)+sizeof(struct tcp_hdr))
      return false;

    tcp_h = (struct tcp_hdr *)(p->payload + sizeof(struct eth_hdr) + sizeof(struct ip_hdr));
/*
    if (ntohs(tcp_h->dest) == HTTP_PORT) {
      curr_mac = mac_h->src;
      curr_origIP = ip_h->dest.addr;
      curr_srcIP = ip_h->src.addr;
      ip_napt_modify_addr_tcp(tcp_h, &ip_h->dest, (uint32_t)myIP);
      ip_napt_modify_addr(ip_h, &ip_h->dest, (uint32_t)myIP);
      return true;
    }
*/
  }

  // let anything else pass
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
struct tcp_hdr *tcp_h;

  if (p->len < sizeof(struct eth_hdr)+sizeof(struct ip_hdr)+sizeof(struct tcp_hdr))
    return true;

  ip_h = (struct ip_hdr *)(p->payload + sizeof(struct eth_hdr));

  if (IPH_PROTO(ip_h) != IP_PROTO_TCP)
    return true;

  tcp_h = (struct tcp_hdr *)(p->payload + sizeof(struct eth_hdr) + sizeof(struct ip_hdr));

  // rewrite packet from our HTTP server
/*  if (ntohs(tcp_h->src) == HTTP_PORT && ip_h->src.addr == (uint32_t)myIP && ip_h->dest.addr == (uint32_t)curr_srcIP) {
    ip_napt_modify_addr_tcp(tcp_h, &ip_h->src, curr_origIP);
    ip_napt_modify_addr(ip_h, &ip_h->src, curr_origIP);
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


void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nNAPT Range extender\n");
  Serial.printf("Heap on start: %d\n", ESP.getFreeHeap());

  // first, connect to STA so we can get a proper local DNS server
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  
  Serial.printf("\nSTA: %s (dns: %s / %s)\n",
                WiFi.localIP().toString().c_str(),
                WiFi.dnsIP(0).toString().c_str(),
                WiFi.dnsIP(1).toString().c_str());

  // give DNS servers to AP side
  dhcps_set_dns(0, WiFi.dnsIP(0));
  dhcps_set_dns(1, WiFi.dnsIP(1));

  WiFi.softAPConfig(  // enable AP, with android-compatible google domain
    IPAddress(172, 217, 28, 254),
    IPAddress(172, 217, 28, 254),
    IPAddress(255, 255, 255, 0));
  WiFi.softAP(STASSID "extender", STAPSK);
  Serial.printf("AP: %s\n", WiFi.softAPIP().toString().c_str());

  Serial.printf("Heap before: %d\n", ESP.getFreeHeap());
  err_t ret = ip_napt_init(NAPT, NAPT_PORT);
  Serial.printf("ip_napt_init(%d,%d): ret=%d (OK=%d)\n", NAPT, NAPT_PORT, (int)ret, (int)ERR_OK);
  if (ret == ERR_OK) {
    ret = ip_napt_enable_no(SOFTAP_IF, 1);
    Serial.printf("ip_napt_enable_no(SOFTAP_IF): ret=%d (OK=%d)\n", (int)ret, (int)ERR_OK);
    if (ret == ERR_OK) {
      Serial.printf("WiFi Network '%s' with same password is now NATed behind '%s'\n", STASSID "extender", STASSID);
    }
  }
  Serial.printf("Heap after napt init: %d\n", ESP.getFreeHeap());
  if (ret != ERR_OK) {
    Serial.printf("NAPT initialization failed\n");
  }
  
  myIP = WiFi.softAPIP();
  // Insert the filter functions
  patch_netif(myIP, my_input_drone, &orig_input_drone, my_output_drone, &orig_output_drone);
}

#else

void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nNAPT not supported in this configuration\n");
}

#endif

void loop() {
}
