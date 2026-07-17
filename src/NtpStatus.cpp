/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 */

#include "NtpStatus.h"
#include "Uptime.h"

#if defined(ESP8266)
#include <coredecls.h>
#elif defined(ESP32)
#include <esp_sntp.h>
#endif

static uint64_t lastSyncMillis = 0;

uint64_t ntpLastSyncMillis() {
	return lastSyncMillis;
}

#if defined(ESP8266)
// from_sntp is false when the clock is set manually (e.g. from the meter
// timestamp), so we only record actual SNTP syncs here.
static void onTimeSync(bool from_sntp) {
	if(from_sntp) lastSyncMillis = millis64();
}
#elif defined(ESP32)
static void onTimeSync(struct timeval* tv) {
	lastSyncMillis = millis64();
}
#endif

void ntpRegisterSyncCallback() {
#if defined(ESP8266)
	settimeofday_cb(onTimeSync);
#elif defined(ESP32)
	sntp_set_time_sync_notification_cb(onTimeSync);
#endif
}
