/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <OffloadControlTestV1_1.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

using android::hardware::tetheroffload::control::V1_1::IOffloadControl;

const hidl_string TEST_IFACE("rmnet_data0");

// Check that calling setDataWarningAndLimit() without first having called initOffload() returns
// false.
TEST_P(OffloadControlTestV1_1_HalNotStarted, SetDataWarningAndLimitWithoutInitReturnsFalse) {
    const Return<void> ret = getControlV1_1()->setDataWarningAndLimit(TEST_IFACE, 5000ULL, 5000ULL,
                                                                      ASSERT_FALSE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

/*
 * Tests for IOffloadControl::setDataWarningAndLimit().
 */

// Test that setDataWarningAndLimit() for an empty interface name fails.
TEST_P(OffloadControlTestV1_1_HalStarted, SetDataWarningAndLimitEmptyUpstreamIfaceFails) {
    const Return<void> ret = getControlV1_1()->setDataWarningAndLimit(
            hidl_string(""), 12345ULL, 67890ULL, ASSERT_FALSE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

// TEST_IFACE is presumed to exist on the device and be up. No packets
// are ever actually caused to be forwarded.
TEST_P(OffloadControlTestV1_1_HalStarted, SetDataWarningAndLimitNonZeroOk) {
    const Return<void> ret = getControlV1_1()->setDataWarningAndLimit(TEST_IFACE, 4000ULL, 5000ULL,
                                                                      ASSERT_TRUE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

// TEST_IFACE is presumed to exist on the device and be up. No packets
// are ever actually caused to be forwarded.
TEST_P(OffloadControlTestV1_1_HalStarted, SetDataWarningAndLimitZeroOk) {
    const Return<void> ret =
            getControlV1_1()->setDataWarningAndLimit(TEST_IFACE, 0ULL, 0ULL, ASSERT_TRUE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(OffloadControlTestV1_1_HalNotStarted);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(OffloadControlTestV1_1_HalStarted);

INSTANTIATE_TEST_CASE_P(
        PerInstance, OffloadControlTestV1_1_HalNotStarted,
        testing::Combine(testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 IOffloadConfig::descriptor)),
                         testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 IOffloadControl::descriptor))),
        android::hardware::PrintInstanceTupleNameToString<>);

INSTANTIATE_TEST_CASE_P(
        PerInstance, OffloadControlTestV1_1_HalStarted,
        testing::Combine(testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 IOffloadConfig::descriptor)),
                         testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 IOffloadControl::descriptor))),
        android::hardware::PrintInstanceTupleNameToString<>);
