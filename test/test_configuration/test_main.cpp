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

void test_wireguard_config_roundtrips_and_tracks_changes(void) {
    NetworkConfig network = makeNetworkConfig(false);
    AmsConfiguration configuration = loadConfigurationWith(network);
    WireGuardConfig wireGuard = {};
    wireGuard.enabled = true;
    strcpy(wireGuard.address, "10.0.0.2");
    strcpy(wireGuard.privateKey, "private-key");
    strcpy(wireGuard.peerPublicKey, "peer-key");
    strcpy(wireGuard.endpoint, "vpn.example.com");
    wireGuard.port = 51820;

    TEST_ASSERT_TRUE(configuration.setWireGuardConfig(wireGuard));
    TEST_ASSERT_TRUE(configuration.isWireGuardConfigChanged());
    configuration.ackWireGuardConfigChange();

    WireGuardConfig loaded;
    TEST_ASSERT_TRUE(configuration.getWireGuardConfig(loaded));
    TEST_ASSERT_TRUE(loaded.enabled);
    TEST_ASSERT_EQUAL_STRING("10.0.0.2", loaded.address);
    TEST_ASSERT_EQUAL_STRING("private-key", loaded.privateKey);
    TEST_ASSERT_EQUAL_STRING("peer-key", loaded.peerPublicKey);
    TEST_ASSERT_EQUAL_STRING("vpn.example.com", loaded.endpoint);
    TEST_ASSERT_EQUAL_UINT16(51820, loaded.port);
    TEST_ASSERT_FALSE(configuration.isWireGuardConfigChanged());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_unchanged_network_config_does_not_mark_network_changed);
    RUN_TEST(test_mdns_change_marks_network_changed);
    RUN_TEST(test_wireguard_config_roundtrips_and_tracks_changes);
    return UNITY_END();
}
