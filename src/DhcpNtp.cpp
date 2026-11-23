/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 * @brief Asks the DHCP server for its NTP server (option 42)
 *
 * @details The lwIP shipped with Arduino-ESP32 2.0.x is precompiled without
 * LWIP_DHCP_GET_NTP_SRV, so its DHCP client never requests option 42 and
 * sntp_servermode_dhcp() is an empty macro. We ask for it ourselves with a
 * DHCPINFORM (RFC 2131 3.4), which a client that already has an address may send
 * to learn further parameters.
 *
 * The reply goes to UDP port 68, which lwIP's DHCP client has bound without
 * SO_REUSEADDR, so we cannot open a socket of our own there. Instead we send
 * from lwIP's DHCP pcb and put a shim in front of its receive callback while the
 * request is in flight. The shim takes the reply matching our transaction id
 * and passes every other packet on to lwIP untouched.
 *
 * Everything that touches the pcb runs on the lwIP thread via tcpip_callback().
 * The state is only moved away from DHCP_NTP_WAITING on the lwIP thread, the
 * main loop only moves it from idle to waiting and from a result back to idle.
 */

#include "DhcpNtp.h"

#if defined(ESP32)

#include <Arduino.h>
#include <string.h>
#include <esp_random.h>
#include <lwip/tcpip.h>
#include <lwip/udp.h>
#include <lwip/netif.h>
#include <lwip/pbuf.h>

#define DHCP_CLIENT_PORT 68
#define DHCP_SERVER_PORT 67
#define DHCP_NTP_TIMEOUT_MS 3000

#define DHCP_OFS_OP 0
#define DHCP_OFS_XID 4
#define DHCP_OFS_CIADDR 12
#define DHCP_OFS_CHADDR 28
#define DHCP_OFS_COOKIE 236
#define DHCP_OFS_OPTIONS 240
#define DHCP_PACKET_LEN 300 // Minimum BOOTP message size, some servers drop shorter ones

static volatile uint8_t state = DHCP_NTP_IDLE;
static volatile uint32_t foundServer = 0;
static volatile bool unhookPosted = false;
static uint32_t targetIp = 0;
static uint32_t startedAt = 0;
static uint8_t xid[4];

// Only touched on the lwIP thread
static struct udp_pcb* hookedPcb = NULL;
static udp_recv_fn origRecv = NULL;
static void* origArg = NULL;

static const uint8_t cookie[4] = { 0x63, 0x82, 0x53, 0x63 };

static void shimRecv(void* arg, struct udp_pcb* pcb, struct pbuf* p, const ip_addr_t* addr, u16_t port);

static bool isHookedPcbAlive() {
	if(hookedPcb == NULL) return false;
	for(struct udp_pcb* pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) {
		if(pcb == hookedPcb) return pcb->recv == shimRecv;
	}
	return false;
}

static void unhook() {
	if(isHookedPcbAlive()) {
		hookedPcb->recv = origRecv;
		hookedPcb->recv_arg = origArg;
	}
	hookedPcb = NULL;
}

static bool isOurReply(struct pbuf* p) {
	if(p->tot_len < DHCP_OFS_OPTIONS + 1) return false;
	if(pbuf_get_at(p, DHCP_OFS_OP) != 2) return false; // BOOTREPLY
	for(uint8_t i = 0; i < 4; i++) {
		if(pbuf_get_at(p, DHCP_OFS_XID + i) != xid[i]) return false;
		if(pbuf_get_at(p, DHCP_OFS_COOKIE + i) != cookie[i]) return false;
	}
	return true;
}

static uint32_t findNtpServer(struct pbuf* p) {
	u16_t i = DHCP_OFS_OPTIONS;
	while(i < p->tot_len) {
		uint8_t code = pbuf_get_at(p, i);
		if(code == 255) break;
		if(code == 0) {
			i++;
			continue;
		}
		if(i + 1 >= p->tot_len) break;
		uint8_t len = pbuf_get_at(p, i + 1);
		if(i + 2 + len > p->tot_len) break;
		if(code == 42 && len >= 4) {
			uint8_t ip[4];
			for(uint8_t j = 0; j < 4; j++) ip[j] = pbuf_get_at(p, i + 2 + j);
			uint32_t server;
			memcpy(&server, ip, 4); // Already in network byte order
			if(server != 0) return server;
		}
		i += 2 + len;
	}
	return 0;
}

static void shimRecv(void* arg, struct udp_pcb* pcb, struct pbuf* p, const ip_addr_t* addr, u16_t port) {
	if(state == DHCP_NTP_WAITING && isOurReply(p)) {
		uint32_t server = findNtpServer(p);
		pbuf_free(p);
		foundServer = server;
		state = server == 0 ? DHCP_NTP_NONE : DHCP_NTP_FOUND;
		unhook();
		return;
	}
	origRecv(origArg, pcb, p, addr, port);
}

static struct pbuf* buildInform(struct netif* nif) {
	struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, DHCP_PACKET_LEN, PBUF_RAM);
	if(p == NULL) return NULL;
	uint8_t* b = (uint8_t*) p->payload;
	memset(b, 0, DHCP_PACKET_LEN);
	b[0] = 1; // BOOTREQUEST
	b[1] = 1; // Ethernet
	b[2] = 6; // Hardware address length
	memcpy(b + DHCP_OFS_XID, xid, 4);
	memcpy(b + DHCP_OFS_CIADDR, &netif_ip4_addr(nif)->addr, 4);
	memcpy(b + DHCP_OFS_CHADDR, nif->hwaddr, nif->hwaddr_len > 16 ? 16 : nif->hwaddr_len);
	memcpy(b + DHCP_OFS_COOKIE, cookie, 4);
	uint8_t* o = b + DHCP_OFS_OPTIONS;
	*o++ = 53; *o++ = 1; *o++ = 8;  // DHCPINFORM
	*o++ = 55; *o++ = 1; *o++ = 42; // Parameter request list: NTP servers
	*o++ = 255;
	return p;
}

static void startOnLwipThread(void* ctx) {
	struct netif* nif = NULL;
	struct netif* n;
	NETIF_FOREACH(n) {
		if(netif_is_up(n) && netif_ip4_addr(n)->addr == targetIp) {
			nif = n;
			break;
		}
	}
	struct udp_pcb* dhcpPcb = NULL;
	for(struct udp_pcb* pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) {
		if(pcb->local_port == DHCP_CLIENT_PORT && pcb->recv != NULL && pcb->recv != shimRecv) {
			dhcpPcb = pcb;
			break;
		}
	}
	if(nif == NULL || dhcpPcb == NULL) {
		state = DHCP_NTP_FAILED;
		return;
	}

	struct pbuf* p = buildInform(nif);
	if(p == NULL) {
		state = DHCP_NTP_FAILED;
		return;
	}

	hookedPcb = dhcpPcb;
	origRecv = dhcpPcb->recv;
	origArg = dhcpPcb->recv_arg;
	dhcpPcb->recv = shimRecv;
	dhcpPcb->recv_arg = NULL;

	err_t err = udp_sendto_if(dhcpPcb, p, IP_ADDR_BROADCAST, DHCP_SERVER_PORT, nif);
	pbuf_free(p);
	if(err != ERR_OK) {
		unhook();
		state = DHCP_NTP_FAILED;
	}
}

static void timeoutOnLwipThread(void* ctx) {
	unhook();
	if(state == DHCP_NTP_WAITING) {
		state = DHCP_NTP_TIMEOUT;
	}
	unhookPosted = false;
}

bool dhcpNtpRequest(uint32_t localIp) {
	if(state != DHCP_NTP_IDLE) return false;
	targetIp = localIp;
	uint32_t r = esp_random();
	memcpy(xid, &r, 4);
	foundServer = 0;
	startedAt = millis();
	state = DHCP_NTP_WAITING;
	if(tcpip_callback(startOnLwipThread, NULL) != ERR_OK) {
		state = DHCP_NTP_FAILED;
	}
	return true;
}

uint8_t dhcpNtpPoll(uint32_t& ntpServer) {
	uint8_t s = state;
	if(s == DHCP_NTP_IDLE) return DHCP_NTP_IDLE;
	if(s == DHCP_NTP_WAITING) {
		if(!unhookPosted && millis() - startedAt > DHCP_NTP_TIMEOUT_MS) {
			unhookPosted = tcpip_callback(timeoutOnLwipThread, NULL) == ERR_OK;
		}
		return DHCP_NTP_WAITING;
	}
	if(unhookPosted) return DHCP_NTP_WAITING; // Let the pending callback run before a new request can start
	ntpServer = foundServer;
	state = DHCP_NTP_IDLE;
	return s;
}

#endif
