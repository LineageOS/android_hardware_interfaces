/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "VtsOffloadControlV1_0TargetTest"

#include <OffloadControlTestV1_0.h>
#include <android-base/stringprintf.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>
#include <net/if.h>
#include <sys/socket.h>

using android::base::StringPrintf;
using android::hardware::Return;
using android::hardware::tetheroffload::config::V1_0::IOffloadConfig;
using android::hardware::tetheroffload::control::V1_0::IOffloadControl;
using android::hardware::Void;

constexpr const char* TEST_IFACE = "rmnet_data0";

// Call initOffload() multiple times. Check that non-first initOffload() calls return false.
TEST_P(OffloadControlTestV1_0_HalNotStarted, AdditionalInitsWithoutStopReturnFalse) {
    initOffload(true);
    initOffload(false);
    initOffload(false);
    initOffload(false);
}

// Check that calling stopOffload() without first having called initOffload() returns false.
TEST_P(OffloadControlTestV1_0_HalNotStarted, MultipleStopsWithoutInitReturnFalse) {
    stopOffload(ExpectBoolean::False);
    stopOffload(ExpectBoolean::False);
    stopOffload(ExpectBoolean::False);
}

// Check whether the specified interface is up.
bool interfaceIsUp(const char* name) {
    if (name == nullptr) return false;
    struct ifreq ifr = {};
    strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
    int sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock == -1) return false;
    int ret = ioctl(sock, SIOCGIFFLAGS, &ifr, sizeof(ifr));
    close(sock);
    return (ret == 0) && (ifr.ifr_flags & IFF_UP);
}

// Check that calling stopOffload() after a complete init/stop cycle returns false.
TEST_P(OffloadControlTestV1_0_HalNotStarted, AdditionalStopsWithInitReturnFalse) {
    initOffload(true);
    // Call setUpstreamParameters() so that "offload" can be reasonably said
    // to be both requested and operational.
    const hidl_string v4Addr("192.0.0.2");
    const hidl_string v4Gw("192.0.0.1");
    const vector<hidl_string> v6Gws{hidl_string("fe80::db8:1"), hidl_string("fe80::db8:2")};
    const Return<void> upstream =
        control->setUpstreamParameters(TEST_IFACE, v4Addr, v4Gw, v6Gws, ASSERT_TRUE_CALLBACK);
    EXPECT_TRUE(upstream.isOk());
    if (!interfaceIsUp(TEST_IFACE)) {
        return;
    }
    SCOPED_TRACE("Expecting stopOffload to succeed");
    stopOffload(ExpectBoolean::Ignored);  // balance out initOffload(true)
    SCOPED_TRACE("Expecting stopOffload to fail the first time");
    stopOffload(ExpectBoolean::False);
    SCOPED_TRACE("Expecting stopOffload to fail the second time");
    stopOffload(ExpectBoolean::False);
}

// Check that calling setLocalPrefixes() without first having called initOffload() returns false.
TEST_P(OffloadControlTestV1_0_HalNotStarted, SetLocalPrefixesWithoutInitReturnsFalse) {
    const vector<hidl_string> prefixes{hidl_string("2001:db8::/64")};
    const Return<void> ret = control->setLocalPrefixes(prefixes, ASSERT_FALSE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

// Check that calling getForwardedStats() without first having called initOffload()
// returns zero bytes statistics.
TEST_P(OffloadControlTestV1_0_HalNotStarted, GetForwardedStatsWithoutInitReturnsZeroValues) {
    const hidl_string upstream(TEST_IFACE);
    const Return<void> ret = control->getForwardedStats(upstream, ASSERT_ZERO_BYTES_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

// Check that calling setDataLimit() without first having called initOffload() returns false.
TEST_P(OffloadControlTestV1_0_HalNotStarted, SetDataLimitWithoutInitReturnsFalse) {
    const hidl_string upstream(TEST_IFACE);
    const uint64_t limit = 5000ULL;
    const Return<void> ret = control->setDataLimit(upstream, limit, ASSERT_FALSE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

// Check that calling setUpstreamParameters() without first having called initOffload()
// returns false.
TEST_P(OffloadControlTestV1_0_HalNotStarted, SetUpstreamParametersWithoutInitReturnsFalse) {
    const hidl_string iface(TEST_IFACE);
    const hidl_string v4Addr("192.0.2.0/24");
    const hidl_string v4Gw("192.0.2.1");
    const vector<hidl_string> v6Gws{hidl_string("fe80::db8:1")};
    const Return<void> ret =
        control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, ASSERT_FALSE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

// Check that calling addDownstream() with an IPv4 prefix without first having called
// initOffload() returns false.
TEST_P(OffloadControlTestV1_0_HalNotStarted, AddIPv4DownstreamWithoutInitReturnsFalse) {
    const hidl_string iface(TEST_IFACE);
    const hidl_string prefix("192.0.2.0/24");
    const Return<void> ret = control->addDownstream(iface, prefix, ASSERT_FALSE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

// Check that calling addDownstream() with an IPv6 prefix without first having called
// initOffload() returns false.
TEST_P(OffloadControlTestV1_0_HalNotStarted, AddIPv6DownstreamWithoutInitReturnsFalse) {
    const hidl_string iface(TEST_IFACE);
    const hidl_string prefix("2001:db8::/64");
    const Return<void> ret = control->addDownstream(iface, prefix, ASSERT_FALSE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

// Check that calling removeDownstream() with an IPv4 prefix without first having called
// initOffload() returns false.
TEST_P(OffloadControlTestV1_0_HalNotStarted, RemoveIPv4DownstreamWithoutInitReturnsFalse) {
    const hidl_string iface(TEST_IFACE);
    const hidl_string prefix("192.0.2.0/24");
    const Return<void> ret = control->removeDownstream(iface, prefix, ASSERT_FALSE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

// Check that calling removeDownstream() with an IPv6 prefix without first having called
// initOffload() returns false.
TEST_P(OffloadControlTestV1_0_HalNotStarted, RemoveIPv6DownstreamWithoutInitReturnsFalse) {
    const hidl_string iface(TEST_IFACE);
    const hidl_string prefix("2001:db8::/64");
    const Return<void> ret = control->removeDownstream(iface, prefix, ASSERT_FALSE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

/*
 * Tests for IOffloadControl::setLocalPrefixes().
 */

// Test setLocalPrefixes() accepts an IPv4 address.
TEST_P(OffloadControlTestV1_0_HalStarted, SetLocalPrefixesIPv4AddressOk) {
    const vector<hidl_string> prefixes{hidl_string("192.0.2.1")};
    const Return<void> ret = control->setLocalPrefixes(prefixes, ASSERT_TRUE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

// Test setLocalPrefixes() accepts an IPv6 address.
TEST_P(OffloadControlTestV1_0_HalStarted, SetLocalPrefixesIPv6AddressOk) {
    const vector<hidl_string> prefixes{hidl_string("fe80::1")};
    const Return<void> ret = control->setLocalPrefixes(prefixes, ASSERT_TRUE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

// Test setLocalPrefixes() accepts both IPv4 and IPv6 prefixes.
TEST_P(OffloadControlTestV1_0_HalStarted, SetLocalPrefixesIPv4v6PrefixesOk) {
    const vector<hidl_string> prefixes{hidl_string("192.0.2.0/24"), hidl_string("fe80::/64")};
    const Return<void> ret = control->setLocalPrefixes(prefixes, ASSERT_TRUE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

// Test that setLocalPrefixes() fails given empty input. There is always
// a non-empty set of local prefixes; when all networking interfaces are down
// we still apply {127.0.0.0/8, ::1/128, fe80::/64} here.
TEST_P(OffloadControlTestV1_0_HalStarted, SetLocalPrefixesEmptyFails) {
    const vector<hidl_string> prefixes{};
    const Return<void> ret = control->setLocalPrefixes(prefixes, ASSERT_FALSE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

// Test setLocalPrefixes() fails on incorrectly formed input strings.
TEST_P(OffloadControlTestV1_0_HalStarted, SetLocalPrefixesInvalidFails) {
    const vector<hidl_string> prefixes{hidl_string("192.0.2.0/24"), hidl_string("invalid")};
    const Return<void> ret = control->setLocalPrefixes(prefixes, ASSERT_FALSE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

/*
 * Tests for IOffloadControl::getForwardedStats().
 */

// Test that getForwardedStats() for a non-existent upstream yields zero bytes statistics.
TEST_P(OffloadControlTestV1_0_HalStarted, GetForwardedStatsInvalidUpstreamIface) {
    const hidl_string upstream("invalid");
    const Return<void> ret = control->getForwardedStats(upstream, ASSERT_ZERO_BYTES_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

// TEST_IFACE is presumed to exist on the device and be up. No packets
// are ever actually caused to be forwarded.
TEST_P(OffloadControlTestV1_0_HalStarted, GetForwardedStatsDummyIface) {
    const hidl_string upstream(TEST_IFACE);
    const Return<void> ret = control->getForwardedStats(upstream, ASSERT_ZERO_BYTES_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

/*
 * Tests for IOffloadControl::setDataLimit().
 */

// Test that setDataLimit() for an empty interface name fails.
TEST_P(OffloadControlTestV1_0_HalStarted, SetDataLimitEmptyUpstreamIfaceFails) {
    const hidl_string upstream("");
    const uint64_t limit = 5000ULL;
    const Return<void> ret = control->setDataLimit(upstream, limit, ASSERT_FALSE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

// TEST_IFACE is presumed to exist on the device and be up. No packets
// are ever actually caused to be forwarded.
TEST_P(OffloadControlTestV1_0_HalStarted, SetDataLimitNonZeroOk) {
    const hidl_string upstream(TEST_IFACE);
    const uint64_t limit = 5000ULL;
    const Return<void> ret = control->setDataLimit(upstream, limit, ASSERT_TRUE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

// TEST_IFACE is presumed to exist on the device and be up. No packets
// are ever actually caused to be forwarded.
TEST_P(OffloadControlTestV1_0_HalStarted, SetDataLimitZeroOk) {
    const hidl_string upstream(TEST_IFACE);
    const uint64_t limit = 0ULL;
    const Return<void> ret = control->setDataLimit(upstream, limit, ASSERT_TRUE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

/*
 * Tests for IOffloadControl::setUpstreamParameters().
 */

// TEST_IFACE is presumed to exist on the device and be up. No packets
// are ever actually caused to be forwarded.
TEST_P(OffloadControlTestV1_0_HalStarted, SetUpstreamParametersIPv6OnlyOk) {
    const hidl_string iface(TEST_IFACE);
    const hidl_string v4Addr("");
    const hidl_string v4Gw("");
    const vector<hidl_string> v6Gws{hidl_string("fe80::db8:1"), hidl_string("fe80::db8:2")};
    const Return<void> ret =
        control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, ASSERT_TRUE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

// TEST_IFACE is presumed to exist on the device and be up. No packets
// are ever actually caused to be forwarded.
TEST_P(OffloadControlTestV1_0_HalStarted, SetUpstreamParametersAlternateIPv6OnlyOk) {
    const hidl_string iface(TEST_IFACE);
    const hidl_string v4Addr;
    const hidl_string v4Gw;
    const vector<hidl_string> v6Gws{hidl_string("fe80::db8:1"), hidl_string("fe80::db8:3")};
    const Return<void> ret =
        control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, ASSERT_TRUE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

// TEST_IFACE is presumed to exist on the device and be up. No packets
// are ever actually caused to be forwarded.
TEST_P(OffloadControlTestV1_0_HalStarted, SetUpstreamParametersIPv4OnlyOk) {
    const hidl_string iface(TEST_IFACE);
    const hidl_string v4Addr("192.0.2.2");
    const hidl_string v4Gw("192.0.2.1");
    const vector<hidl_string> v6Gws{};
    const Return<void> ret =
        control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, ASSERT_TRUE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

// TEST_IFACE is presumed to exist on the device and be up. No packets
// are ever actually caused to be forwarded.
TEST_P(OffloadControlTestV1_0_HalStarted, SetUpstreamParametersIPv4v6Ok) {
    const hidl_string iface(TEST_IFACE);
    const hidl_string v4Addr("192.0.2.2");
    const hidl_string v4Gw("192.0.2.1");
    const vector<hidl_string> v6Gws{hidl_string("fe80::db8:1"), hidl_string("fe80::db8:2")};
    const Return<void> ret =
        control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, ASSERT_TRUE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

// Test that setUpstreamParameters() fails when all parameters are empty.
TEST_P(OffloadControlTestV1_0_HalStarted, SetUpstreamParametersEmptyFails) {
    const hidl_string iface("");
    const hidl_string v4Addr("");
    const hidl_string v4Gw("");
    const vector<hidl_string> v6Gws{};
    const Return<void> ret =
        control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, ASSERT_FALSE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

// Test that setUpstreamParameters() fails when given empty or non-existent interface names.
TEST_P(OffloadControlTestV1_0_HalStarted, SetUpstreamParametersBogusIfaceFails) {
    const hidl_string v4Addr("192.0.2.2");
    const hidl_string v4Gw("192.0.2.1");
    const vector<hidl_string> v6Gws{hidl_string("fe80::db8:1")};
    for (const auto& bogus : {"", "invalid"}) {
        SCOPED_TRACE(StringPrintf("iface='%s'", bogus));
        const hidl_string iface(bogus);
        const Return<void> ret =
            control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, ASSERT_FALSE_CALLBACK);
        EXPECT_TRUE(ret.isOk());
    }
}

// Test that setUpstreamParameters() fails when given unparseable IPv4 addresses.
TEST_P(OffloadControlTestV1_0_HalStarted, SetUpstreamParametersInvalidIPv4AddrFails) {
    const hidl_string iface(TEST_IFACE);
    const hidl_string v4Gw("192.0.2.1");
    const vector<hidl_string> v6Gws{hidl_string("fe80::db8:1")};
    for (const auto& bogus : {"invalid", "192.0.2"}) {
        SCOPED_TRACE(StringPrintf("v4addr='%s'", bogus));
        const hidl_string v4Addr(bogus);
        const Return<void> ret =
            control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, ASSERT_FALSE_CALLBACK);
        EXPECT_TRUE(ret.isOk());
    }
}

// Test that setUpstreamParameters() fails when given unparseable IPv4 gateways.
TEST_P(OffloadControlTestV1_0_HalStarted, SetUpstreamParametersInvalidIPv4GatewayFails) {
    const hidl_string iface(TEST_IFACE);
    const hidl_string v4Addr("192.0.2.2");
    const vector<hidl_string> v6Gws{hidl_string("fe80::db8:1")};
    for (const auto& bogus : {"invalid", "192.0.2"}) {
        SCOPED_TRACE(StringPrintf("v4gateway='%s'", bogus));
        const hidl_string v4Gw(bogus);
        const Return<void> ret =
            control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, ASSERT_FALSE_CALLBACK);
        EXPECT_TRUE(ret.isOk());
    }
}

// Test that setUpstreamParameters() fails when given unparseable IPv6 gateways.
TEST_P(OffloadControlTestV1_0_HalStarted, SetUpstreamParametersBadIPv6GatewaysFail) {
    const hidl_string iface(TEST_IFACE);
    const hidl_string v4Addr("192.0.2.2");
    const hidl_string v4Gw("192.0.2.1");
    for (const auto& bogus : {"", "invalid", "fe80::bogus", "192.0.2.66"}) {
        SCOPED_TRACE(StringPrintf("v6gateway='%s'", bogus));
        const vector<hidl_string> v6Gws{hidl_string("fe80::1"), hidl_string(bogus)};
        const Return<void> ret =
            control->setUpstreamParameters(iface, v4Addr, v4Gw, v6Gws, ASSERT_FALSE_CALLBACK);
        EXPECT_TRUE(ret.isOk());
    }
}

/*
 * Tests for IOffloadControl::addDownstream().
 */

// Test addDownstream() works given an IPv4 prefix.
TEST_P(OffloadControlTestV1_0_HalStarted, AddDownstreamIPv4) {
    const hidl_string iface("dummy0");
    const hidl_string prefix("192.0.2.0/24");
    const Return<void> ret = control->addDownstream(iface, prefix, ASSERT_TRUE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

// Test addDownstream() works given an IPv6 prefix.
TEST_P(OffloadControlTestV1_0_HalStarted, AddDownstreamIPv6) {
    const hidl_string iface("dummy0");
    const hidl_string prefix("2001:db8::/64");
    const Return<void> ret = control->addDownstream(iface, prefix, ASSERT_TRUE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

// Test addDownstream() fails given all empty parameters.
TEST_P(OffloadControlTestV1_0_HalStarted, AddDownstreamEmptyFails) {
    const hidl_string iface("");
    const hidl_string prefix("");
    const Return<void> ret = control->addDownstream(iface, prefix, ASSERT_FALSE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

// Test addDownstream() fails given empty or non-existent interface names.
TEST_P(OffloadControlTestV1_0_HalStarted, AddDownstreamInvalidIfaceFails) {
    const hidl_string prefix("192.0.2.0/24");
    for (const auto& bogus : {"", "invalid"}) {
        SCOPED_TRACE(StringPrintf("iface='%s'", bogus));
        const hidl_string iface(bogus);
        const Return<void> ret = control->addDownstream(iface, prefix, ASSERT_FALSE_CALLBACK);
        EXPECT_TRUE(ret.isOk());
    }
}

// Test addDownstream() fails given unparseable prefix arguments.
TEST_P(OffloadControlTestV1_0_HalStarted, AddDownstreamBogusPrefixFails) {
    const hidl_string iface("dummy0");
    for (const auto& bogus : {"", "192.0.2/24", "2001:db8/64"}) {
        SCOPED_TRACE(StringPrintf("prefix='%s'", bogus));
        const hidl_string prefix(bogus);
        const Return<void> ret = control->addDownstream(iface, prefix, ASSERT_FALSE_CALLBACK);
        EXPECT_TRUE(ret.isOk());
    }
}

/*
 * Tests for IOffloadControl::removeDownstream().
 */

// Test removeDownstream() works given an IPv4 prefix.
TEST_P(OffloadControlTestV1_0_HalStarted, RemoveDownstreamIPv4) {
    const hidl_string iface("dummy0");
    const hidl_string prefix("192.0.2.0/24");
    // First add the downstream, otherwise removeDownstream logic can reasonably
    // return false for downstreams not previously added.
    const Return<void> add = control->addDownstream(iface, prefix, ASSERT_TRUE_CALLBACK);
    EXPECT_TRUE(add.isOk());
    const Return<void> del = control->removeDownstream(iface, prefix, ASSERT_TRUE_CALLBACK);
    EXPECT_TRUE(del.isOk());
}

// Test removeDownstream() works given an IPv6 prefix.
TEST_P(OffloadControlTestV1_0_HalStarted, RemoveDownstreamIPv6) {
    const hidl_string iface("dummy0");
    const hidl_string prefix("2001:db8::/64");
    // First add the downstream, otherwise removeDownstream logic can reasonably
    // return false for downstreams not previously added.
    const Return<void> add = control->addDownstream(iface, prefix, ASSERT_TRUE_CALLBACK);
    EXPECT_TRUE(add.isOk());
    const Return<void> del = control->removeDownstream(iface, prefix, ASSERT_TRUE_CALLBACK);
    EXPECT_TRUE(del.isOk());
}

// Test removeDownstream() fails given all empty parameters.
TEST_P(OffloadControlTestV1_0_HalStarted, RemoveDownstreamEmptyFails) {
    const hidl_string iface("");
    const hidl_string prefix("");
    const Return<void> ret = control->removeDownstream(iface, prefix, ASSERT_FALSE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

// Test removeDownstream() fails given empty or non-existent interface names.
TEST_P(OffloadControlTestV1_0_HalStarted, RemoveDownstreamBogusIfaceFails) {
    const hidl_string prefix("192.0.2.0/24");
    for (const auto& bogus : {"", "invalid"}) {
        SCOPED_TRACE(StringPrintf("iface='%s'", bogus));
        const hidl_string iface(bogus);
        const Return<void> ret = control->removeDownstream(iface, prefix, ASSERT_FALSE_CALLBACK);
        EXPECT_TRUE(ret.isOk());
    }
}

// Test removeDownstream() fails given unparseable prefix arguments.
TEST_P(OffloadControlTestV1_0_HalStarted, RemoveDownstreamBogusPrefixFails) {
    const hidl_string iface("dummy0");
    for (const auto& bogus : {"", "192.0.2/24", "2001:db8/64"}) {
        SCOPED_TRACE(StringPrintf("prefix='%s'", bogus));
        const hidl_string prefix(bogus);
        const Return<void> ret = control->removeDownstream(iface, prefix, ASSERT_FALSE_CALLBACK);
        EXPECT_TRUE(ret.isOk());
    }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(OffloadControlTestV1_0_HalNotStarted);
INSTANTIATE_TEST_CASE_P(
        PerInstance, OffloadControlTestV1_0_HalNotStarted,
        testing::Combine(testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 IOffloadConfig::descriptor)),
                         testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 IOffloadControl::descriptor))),
        android::hardware::PrintInstanceTupleNameToString<>);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(OffloadControlTestV1_0_HalStarted);
INSTANTIATE_TEST_CASE_P(
        PerInstance, OffloadControlTestV1_0_HalStarted,
        testing::Combine(testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 IOffloadConfig::descriptor)),
                         testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 IOffloadControl::descriptor))),
        android::hardware::PrintInstanceTupleNameToString<>);
