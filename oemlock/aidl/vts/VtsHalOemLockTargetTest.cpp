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
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>

#include <aidl/android/hardware/oemlock/IOemLock.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using ::aidl::android::hardware::oemlock::IOemLock;
using ::aidl::android::hardware::oemlock::OemLockSecureStatus;

using ndk::SpAIBinder;

struct OemLockAidlTest : public ::testing::TestWithParam<std::string> {
    virtual void SetUp() override {
        oemlock = IOemLock::fromBinder(
            SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
        ASSERT_NE(oemlock, nullptr);
    }

    virtual void TearDown() override {}

    std::shared_ptr<IOemLock> oemlock;
};

/*
 * Check the name can be retrieved
 */
TEST_P(OemLockAidlTest, GetName) {
    std::string name;

    const auto ret = oemlock->getName(&name);

    ASSERT_TRUE(ret.isOk());
    // Any value acceptable
};

/*
 * Check the unlock allowed by device state can be queried
 */
TEST_P(OemLockAidlTest, QueryUnlockAllowedByDevice) {
    bool allowed;

    const auto ret = oemlock->isOemUnlockAllowedByDevice(&allowed);

    ASSERT_TRUE(ret.isOk());
    // Any value acceptable
}

/*
 * Check unlock allowed by device state can be toggled
 */
TEST_P(OemLockAidlTest, AllowedByDeviceCanBeToggled) {
    bool allowed;

    // Get the original state so it can be restored
    const auto get_ret = oemlock->isOemUnlockAllowedByDevice(&allowed);
    ASSERT_TRUE(get_ret.isOk());
    const bool originallyAllowed = allowed;

    // Toggle the state
    const auto set_ret = oemlock->setOemUnlockAllowedByDevice(!originallyAllowed);
    ASSERT_TRUE(set_ret.isOk());

    const auto check_set_ret = oemlock->isOemUnlockAllowedByDevice(&allowed);
    ASSERT_TRUE(check_set_ret.isOk());
    ASSERT_EQ(allowed, !originallyAllowed);

    // Restore the state
    const auto restore_ret = oemlock->setOemUnlockAllowedByDevice(originallyAllowed);
    ASSERT_TRUE(restore_ret.isOk());

    const auto check_restore_ret = oemlock->isOemUnlockAllowedByDevice(&allowed);
    ASSERT_TRUE(check_restore_ret.isOk());
    ASSERT_EQ(allowed, originallyAllowed);
}

/*
 * Check the unlock allowed by device state can be queried
 */
TEST_P(OemLockAidlTest, QueryUnlockAllowedByCarrier) {
    bool allowed;

    const auto ret = oemlock->isOemUnlockAllowedByCarrier(&allowed);

    ASSERT_TRUE(ret.isOk());
    // Any value acceptable
}

/*
 * Attempt to check unlock allowed by carrier can be toggled
 *
 * The implementation may involve a signature which cannot be tested here. That
 * is a valid implementation so the test will pass. If there is no signature
 * required, the test will toggle the value.
 */
TEST_P(OemLockAidlTest, CarrierUnlock) {
    const std::vector<uint8_t> noSignature = {};
    bool allowed;
    OemLockSecureStatus secure_status;

    // Get the original state so it can be restored
    const auto get_ret = oemlock->isOemUnlockAllowedByCarrier(&allowed);
    ASSERT_TRUE(get_ret.isOk());
    const bool originallyAllowed = allowed;

    if (originallyAllowed) {
       // Only applied to locked devices
       return;
    }

    // Toggle the state
    const auto set_ret = oemlock->setOemUnlockAllowedByCarrier(!originallyAllowed, noSignature, &secure_status);
    ASSERT_TRUE(set_ret.isOk());
    ASSERT_NE(secure_status, OemLockSecureStatus::FAILED);
    const auto set_status = secure_status;

    const auto check_set_ret = oemlock->isOemUnlockAllowedByCarrier(&allowed);
    ASSERT_TRUE(check_set_ret.isOk());

    if (set_status == OemLockSecureStatus::INVALID_SIGNATURE) {
       // Signature is required so we cannot toggle the value in the test, but this is allowed
       ASSERT_EQ(allowed, originallyAllowed);
       return;
    }

    ASSERT_EQ(set_status, OemLockSecureStatus::OK);
    ASSERT_EQ(allowed, !originallyAllowed);

    // Restore the state
    const auto restore_ret = oemlock->setOemUnlockAllowedByCarrier(originallyAllowed, noSignature, &secure_status);
    ASSERT_TRUE(restore_ret.isOk());
    ASSERT_EQ(secure_status, OemLockSecureStatus::OK);

    const auto check_restore_ret = oemlock->isOemUnlockAllowedByCarrier(&allowed);
    ASSERT_TRUE(check_restore_ret.isOk());
    ASSERT_EQ(allowed, originallyAllowed);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(OemLockAidlTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, OemLockAidlTest,
        testing::ValuesIn(android::getAidlHalInstanceNames(IOemLock::descriptor)),
        android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
