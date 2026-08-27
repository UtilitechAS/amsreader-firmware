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

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_unchanged_network_config_does_not_mark_network_changed);
    RUN_TEST(test_mdns_change_marks_network_changed);
    return UNITY_END();
}
