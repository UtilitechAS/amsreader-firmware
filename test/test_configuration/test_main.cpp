/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 */

#include <unity.h>

#include "AmsConfiguration.h"

void setUp(void) {
    EEPROM.reset();
}

void tearDown(void) {}

static AmsConfiguration loadConfigurationWith(NetworkConfig& network) {
    uint8_t version = EEPROM_EXPECTED_VERSION;
    EEPROM.put(EEPROM_CONFIG_ADDRESS, version);
    EEPROM.put(CONFIG_NETWORK_START, network);

    AmsConfiguration configuration;
    configuration.load();
    configuration.ackNetworkConfigChange();
    return configuration;
}

static NetworkConfig makeNetworkConfig(bool mdns) {
    NetworkConfig network = {};
    network.mdns = mdns;
    network.mode = 1;
    network.sleep = 1;
    return network;
}

void test_unchanged_network_config_does_not_mark_network_changed(void) {
    NetworkConfig network = makeNetworkConfig(false);
    AmsConfiguration configuration = loadConfigurationWith(network);

    TEST_ASSERT_TRUE(configuration.hasConfig());
    TEST_ASSERT_FALSE(configuration.setNetworkConfig(network));
    TEST_ASSERT_FALSE(configuration.isNetworkConfigChanged());
}

void test_mdns_change_marks_network_changed(void) {
    NetworkConfig network = makeNetworkConfig(false);
    AmsConfiguration configuration = loadConfigurationWith(network);
    network.mdns = true;

    TEST_ASSERT_TRUE(configuration.hasConfig());
    TEST_ASSERT_TRUE(configuration.setNetworkConfig(network));
    TEST_ASSERT_TRUE(configuration.isNetworkConfigChanged());
}

static NetworkConfig makeAccessPointConfig(const char* psk) {
    NetworkConfig network = makeNetworkConfig(false);
    network.mode = 2;
    strcpy(network.ssid, "reader");
    strcpy(network.psk, psk);
    return network;
}

static uint8_t storedNetworkMode(AmsConfiguration& configuration) {
    NetworkConfig stored;
    configuration.getNetworkConfig(stored);
    return stored.mode;
}

void test_access_point_psk_length(void) {
    TEST_ASSERT_FALSE(AmsConfiguration::isValidApPsk(""));
    TEST_ASSERT_FALSE(AmsConfiguration::isValidApPsk("1234567"));
    TEST_ASSERT_TRUE(AmsConfiguration::isValidApPsk("12345678"));
}

void test_access_point_without_psk_is_not_stored(void) {
    NetworkConfig client = makeNetworkConfig(false);
    AmsConfiguration configuration = loadConfigurationWith(client);

    NetworkConfig open = makeAccessPointConfig("");
    TEST_ASSERT_FALSE(configuration.setNetworkConfig(open));
    NetworkConfig tooShort = makeAccessPointConfig("1234567");
    TEST_ASSERT_FALSE(configuration.setNetworkConfig(tooShort));

    TEST_ASSERT_FALSE(configuration.isNetworkConfigChanged());
    TEST_ASSERT_EQUAL_UINT8(1, storedNetworkMode(configuration));
}

void test_access_point_with_psk_is_stored(void) {
    NetworkConfig client = makeNetworkConfig(false);
    AmsConfiguration configuration = loadConfigurationWith(client);

    NetworkConfig ap = makeAccessPointConfig("12345678");
    TEST_ASSERT_TRUE(configuration.setNetworkConfig(ap));
    TEST_ASSERT_EQUAL_UINT8(2, storedNetworkMode(configuration));
}

void test_already_open_access_point_can_still_save_other_fields(void) {
    NetworkConfig open = makeAccessPointConfig("");
    AmsConfiguration configuration = loadConfigurationWith(open);

    strcpy(open.hostname, "renamed");
    TEST_ASSERT_TRUE(configuration.setNetworkConfig(open));

    NetworkConfig stored;
    configuration.getNetworkConfig(stored);
    TEST_ASSERT_EQUAL_STRING("renamed", stored.hostname);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_unchanged_network_config_does_not_mark_network_changed);
    RUN_TEST(test_mdns_change_marks_network_changed);
    RUN_TEST(test_access_point_psk_length);
    RUN_TEST(test_access_point_without_psk_is_not_stored);
    RUN_TEST(test_access_point_with_psk_is_stored);
    RUN_TEST(test_already_open_access_point_can_still_save_other_fields);
    return UNITY_END();
}
