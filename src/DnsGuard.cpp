/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 * 
 * @brief Keeps the IPv4 DNS servers from being evicted by IPv6 ones
 * 
 * @details lwIP keeps a single global DNS server table, and both nd6 RDNSS and
 * stateless DHCPv6 start writing IPv6 servers at index 0, evicting the IPv4
 * servers handed out by DHCP. RDNSS is re-applied on every router
 * advertisement, and those are processed even when IPv6 is disabled in our
 * configuration, since lwIP is always joined to the link-local all-nodes group.
 * 
 * We therefore reserve the first slots of the table for IPv4 and the last one
 * for IPv6, which lets both stacks resolve names on a dual stack network.
 * 
 * The table is touched from the lwIP thread, the network event handler and the
 * main loop. All the state here is word sized and every disagreement is fixed
 * by the next reconcile, so no locking is needed. Taking one on the lwIP thread
 * would not be safe anyway.
 */

#include "DnsGuard.h"

#if defined(ESP32)

#include <lwip/dns.h>

#if DNS_MAX_SERVERS < 3
#error "DnsGuard needs room for both an IPv4 and an IPv6 DNS server"
#endif

// Slots reserved for IPv4, the remaining one is where IPv6 is parked
#define DNS_V4_SLOTS (DNS_MAX_SERVERS - 1)

extern "C" void __real_dns_setserver(u8_t numdns, const ip_addr_t *dnsserver);

static ip4_addr_t knownDns[DNS_V4_SLOTS];
static bool ipv6Allowed = false;
static uint32_t repairs = 0;

static bool isUsableV4(const ip_addr_t* server) {
	return server != NULL && IP_IS_V4(server) && !ip_addr_isany(server);
}

static bool hasV4Server() {
	for(uint8_t i = 0; i < DNS_MAX_SERVERS; i++) {
		if(isUsableV4(dns_getserver(i))) return true;
	}
	return false;
}

#if defined(AMS_WRAP_DNS_SETSERVER)
/**
 * Intercepts every write to the DNS table. Runs on the lwIP thread, so it must
 * not block or log.
 */
extern "C" void __wrap_dns_setserver(u8_t numdns, const ip_addr_t *dnsserver) {
	// Only displace an IPv6 server when there is an IPv4 one worth protecting
	if(dnsserver != NULL && IP_IS_V6(dnsserver) && hasV4Server()) {
		if(ipv6Allowed) {
			__real_dns_setserver(DNS_MAX_SERVERS - 1, dnsserver);
		}
		return;
	}
	__real_dns_setserver(numdns, dnsserver);
}
#endif

/**
 * Remember the IPv4 servers, but only while the IPv4 slots are untainted, so a
 * table that has already been overwritten can never become the source of truth.
 * The IPv6 slot is deliberately not read here.
 */
static void remember() {
	for(uint8_t i = 0; i < DNS_V4_SLOTS; i++) {
		if(IP_IS_V6(dns_getserver(i))) return;
	}

	ip4_addr_t found[DNS_V4_SLOTS];
	uint8_t count = 0;
	for(uint8_t i = 0; i < DNS_V4_SLOTS; i++) {
		const ip_addr_t* server = dns_getserver(i);
		if(isUsableV4(server)) {
			found[count++] = *ip_2_ip4(server);
		}
	}
	if(count == 0) return;

	for(uint8_t i = 0; i < DNS_V4_SLOTS; i++) {
		if(i < count) {
			knownDns[i] = found[i];
		} else {
			ip4_addr_set_zero(&knownDns[i]);
		}
	}
}

void dnsGuardSetIpv6Allowed(bool allowed) {
	ipv6Allowed = allowed;
}

void dnsGuardEnforce() {
	remember();

	ip_addr_t desired[DNS_MAX_SERVERS];
	uint8_t count = 0;
	for(uint8_t i = 0; i < DNS_V4_SLOTS; i++) {
		if(!ip4_addr_isany_val(knownDns[i])) {
			ip_addr_copy_from_ip4(desired[count], knownDns[i]);
			count++;
		}
	}
	if(count == 0) return; // No IPv4 DNS known, leave an IPv6 only or closed network alone

	if(ipv6Allowed) {
		for(uint8_t i = 0; i < DNS_MAX_SERVERS; i++) {
			const ip_addr_t* server = dns_getserver(i);
			if(server != NULL && IP_IS_V6(server) && !ip_addr_isany(server)) {
				ip_addr_copy(desired[count], *server);
				count++;
				break;
			}
		}
	}
	while(count < DNS_MAX_SERVERS) {
		ip_addr_set_zero(&desired[count]);
		count++;
	}

	bool repaired = false;
	for(uint8_t i = 0; i < DNS_MAX_SERVERS; i++) {
		if(!ip_addr_cmp(dns_getserver(i), &desired[i])) {
			dns_setserver(i, &desired[i]);
			repaired = true;
		}
	}
	if(repaired) repairs++;
}

uint32_t dnsGuardRepairs() {
	return repairs;
}

#endif
