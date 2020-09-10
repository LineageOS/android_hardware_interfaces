/*
 * Copyright (C) 2019 The Android Open Source Project
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

#define LOG_TAG "VtsHalUsbV1_2TargetTest"
#include <android-base/logging.h>

#include <android/hardware/usb/1.2/IUsb.h>
#include <android/hardware/usb/1.2/IUsbCallback.h>
#include <android/hardware/usb/1.2/types.h>

#include <VtsHalHidlTargetCallbackBase.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include <log/log.h>
#include <stdlib.h>
#include <chrono>
#include <condition_variable>
#include <mutex>

using ::android::sp;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::usb::V1_0::PortDataRole;
using ::android::hardware::usb::V1_0::PortMode;
using ::android::hardware::usb::V1_0::PortPowerRole;
using ::android::hardware::usb::V1_0::PortRole;
using ::android::hardware::usb::V1_0::PortRoleType;
using ::android::hardware::usb::V1_0::Status;
using ::android::hardware::usb::V1_1::PortMode_1_1;
using ::android::hardware::usb::V1_1::PortStatus_1_1;
using ::android::hardware::usb::V1_2::ContaminantDetectionStatus;
using ::android::hardware::usb::V1_2::ContaminantProtectionMode;
using ::android::hardware::usb::V1_2::ContaminantProtectionStatus;
using ::android::hardware::usb::V1_2::IUsb;
using ::android::hardware::usb::V1_2::IUsbCallback;
using ::android::hardware::usb::V1_2::PortStatus;
using ::android::hidl::base::V1_0::IBase;

constexpr char kCallbackNameNotifyPortStatusChange_1_2[] = "notifyPortStatusChange_1_2";
const int kCallbackIdentifier = 2;

// Worst case wait time 20secs
#define WAIT_FOR_TIMEOUT std::chrono::milliseconds(20000)

class UsbClientCallbackArgs {
   public:
    // The last conveyed status of the USB ports.
    // Stores information of currentt_data_role, power_role for all the USB ports
    PortStatus usb_last_port_status;

    // Status of the last role switch operation.
    Status usb_last_status;

    // Identifier for the usb callback object.
    // Stores the cookie of the last invoked usb callback object.
    int last_usb_cookie;
};

// Callback class for the USB HIDL hal.
// Usb Hal will call this object upon role switch or port query.
class UsbCallback : public ::testing::VtsHalHidlTargetCallbackBase<UsbClientCallbackArgs>,
                    public IUsbCallback {
    int cookie;

   public:
    UsbCallback(int cookie) : cookie(cookie){};

    virtual ~UsbCallback() = default;

    // V1_0 Callback method for the port status.
    // This should not be called so not signalling the Test here assuming that
    // the test thread will timeout
    Return<void> notifyPortStatusChange(const hidl_vec<android::hardware::usb::V1_0::PortStatus>&
                                        /* currentPortStatus */,
                                        Status /* retval */) override {
        return Void();
    };

    // V1_1 Callback method for the port status.
    // This should not be called so not signalling the Test here assuming that
    // the test thread will timeout
    Return<void> notifyPortStatusChange_1_1(const hidl_vec<PortStatus_1_1>& /* currentPortStatus */,
                                            Status /* retval */) override {
        return Void();
    }

    // This callback method should be used.
    Return<void> notifyPortStatusChange_1_2(const hidl_vec<PortStatus>& currentPortStatus,
                                            Status retval) override {
        UsbClientCallbackArgs arg;
        if (retval == Status::SUCCESS) {
            arg.usb_last_port_status.status_1_1.status.supportedModes =
                    currentPortStatus[0].status_1_1.status.supportedModes;
            arg.usb_last_port_status.status_1_1.status.currentMode =
                    currentPortStatus[0].status_1_1.status.currentMode;
            arg.usb_last_port_status.status_1_1.status.portName =
                    currentPortStatus[0].status_1_1.status.portName;
            arg.usb_last_port_status.contaminantDetectionStatus =
                    currentPortStatus[0].contaminantDetectionStatus;
            arg.usb_last_port_status.contaminantProtectionStatus =
                    currentPortStatus[0].contaminantProtectionStatus;
            arg.usb_last_port_status.supportsEnableContaminantPresenceProtection =
                    currentPortStatus[0].supportsEnableContaminantPresenceProtection;
            arg.usb_last_port_status.supportsEnableContaminantPresenceDetection =
                    currentPortStatus[0].supportsEnableContaminantPresenceDetection;
            arg.usb_last_port_status.supportedContaminantProtectionModes =
                    currentPortStatus[0].supportedContaminantProtectionModes;
        }
        arg.usb_last_status = retval;
        arg.last_usb_cookie = cookie;

        NotifyFromCallback(kCallbackNameNotifyPortStatusChange_1_2, arg);
        return Void();
    }

    // Callback method for the status of role switch operation.
    // RoleSwitch operation has not changed since V1_0 so leaving
    // the callback blank here.
    Return<void> notifyRoleSwitchStatus(const hidl_string& /*portName*/,
                                        const PortRole& /*newRole*/, Status /*retval*/) override {
        return Void();
    };
};

// The main test class for the USB hidl HAL
class UsbHidlTest : public ::testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        ALOGI(__FUNCTION__);
        usb = IUsb::getService(GetParam());
        ASSERT_NE(usb, nullptr);

        usb_cb_2 = new UsbCallback(kCallbackIdentifier);
        ASSERT_NE(usb_cb_2, nullptr);
        usb_cb_2->SetWaitTimeout(kCallbackNameNotifyPortStatusChange_1_2, WAIT_FOR_TIMEOUT);
        Return<void> ret = usb->setCallback(usb_cb_2);
        ASSERT_TRUE(ret.isOk());
    }

    virtual void TearDown() override { ALOGI("Teardown"); }

    // USB hidl hal Proxy
    sp<IUsb> usb;

    // Callback objects for usb hidl
    // Methods of these objects are called to notify port status updates.
    sp<UsbCallback> usb_cb_1;
    sp<UsbCallback> usb_cb_2;
};

/*
 * Test to see if setCallback on V1_1 callback object succeeds.
 * Callback oject is created and registered.
 * Check to see if the hidl transaction succeeded.
 */
TEST_P(UsbHidlTest, setCallback) {
    usb_cb_1 = new UsbCallback(1);
    ASSERT_NE(usb_cb_1, nullptr);
    Return<void> ret = usb->setCallback(usb_cb_1);
    ASSERT_TRUE(ret.isOk());
}

/*
 * Check to see if querying type-c
 * port status succeeds.
 * HAL service should call notifyPortStatusChange_1_2
 * instead of notifyPortStatusChange of V1_0/V1_1 interface
 */
TEST_P(UsbHidlTest, queryPortStatus) {
    Return<void> ret = usb->queryPortStatus();
    ASSERT_TRUE(ret.isOk());
    auto res = usb_cb_2->WaitForCallback(kCallbackNameNotifyPortStatusChange_1_2);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(kCallbackIdentifier, res.args->last_usb_cookie);
    EXPECT_EQ(PortMode::NONE, res.args->usb_last_port_status.status_1_1.status.currentMode);
    EXPECT_EQ(PortMode::NONE, res.args->usb_last_port_status.status_1_1.status.supportedModes);
    EXPECT_EQ(Status::SUCCESS, res.args->usb_last_status);
}

/*
 * supportedContaminantProtectionModes is immutable.
 * Check if supportedContaminantProtectionModes changes across queryPortStatus
 * call.
 */
TEST_P(UsbHidlTest, checkSupportedContaminantProtectionModes) {
    Return<void> ret = usb->queryPortStatus();
    ASSERT_TRUE(ret.isOk());
    auto res = usb_cb_2->WaitForCallback(kCallbackNameNotifyPortStatusChange_1_2);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(kCallbackIdentifier, res.args->last_usb_cookie);
    EXPECT_EQ(PortMode::NONE, res.args->usb_last_port_status.status_1_1.status.currentMode);
    EXPECT_EQ(PortMode::NONE, res.args->usb_last_port_status.status_1_1.status.supportedModes);
    EXPECT_EQ(Status::SUCCESS, res.args->usb_last_status);

    uint32_t supportedContaminantProtectionModes = static_cast<uint32_t>(
            res.args->usb_last_port_status.supportedContaminantProtectionModes);
    for (int runs = 1; runs <= 10; runs++) {
        ret = usb->queryPortStatus();
        ASSERT_TRUE(ret.isOk());
        res = usb_cb_2->WaitForCallback(kCallbackNameNotifyPortStatusChange_1_2);
        EXPECT_TRUE(res.no_timeout);
        EXPECT_EQ(kCallbackIdentifier, res.args->last_usb_cookie);
        EXPECT_EQ(PortMode::NONE, res.args->usb_last_port_status.status_1_1.status.currentMode);
        EXPECT_EQ(PortMode::NONE, res.args->usb_last_port_status.status_1_1.status.supportedModes);
        EXPECT_EQ(Status::SUCCESS, res.args->usb_last_status);
        EXPECT_EQ(supportedContaminantProtectionModes,
                  static_cast<uint32_t>(
                          res.args->usb_last_port_status.supportedContaminantProtectionModes));
    }
}

/*
 * When supportsEnableContaminantPresenceDetection is set false,
 * enableContaminantPresenceDetection should not enable/disable
 * contaminantPresenceProtection.
 */
TEST_P(UsbHidlTest, presenceDetectionSupportedCheck) {
    Return<void> ret = usb->queryPortStatus();
    ASSERT_TRUE(ret.isOk());
    auto res = usb_cb_2->WaitForCallback(kCallbackNameNotifyPortStatusChange_1_2);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(kCallbackIdentifier, res.args->last_usb_cookie);
    EXPECT_EQ(Status::SUCCESS, res.args->usb_last_status);

    if (!res.args->usb_last_port_status.supportsEnableContaminantPresenceDetection) {
        for (int runs = 1; runs <= 10; runs++) {
            bool currentStatus = !(res.args->usb_last_port_status.contaminantDetectionStatus ==
                                   ContaminantDetectionStatus::DISABLED);

            ret = usb->enableContaminantPresenceDetection(
                    res.args->usb_last_port_status.status_1_1.status.portName, !currentStatus);
            ASSERT_TRUE(ret.isOk());

            res = usb_cb_2->WaitForCallback(kCallbackNameNotifyPortStatusChange_1_2);
            EXPECT_TRUE(res.no_timeout);
            EXPECT_EQ(kCallbackIdentifier, res.args->last_usb_cookie);
            EXPECT_EQ(currentStatus, !(res.args->usb_last_port_status.contaminantDetectionStatus ==
                                       ContaminantDetectionStatus::DISABLED));
        }
    }
}

/*
 * enableContaminantPresenceDetection should succeed atleast 90% when supported.
 */
TEST_P(UsbHidlTest, contaminantPresenceDetectionStability) {
    int successCount = 0;
    bool currentStatus;
    bool supported = true;

    Return<void> ret = usb->queryPortStatus();
    ASSERT_TRUE(ret.isOk());
    auto res = usb_cb_2->WaitForCallback(kCallbackNameNotifyPortStatusChange_1_2);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(kCallbackIdentifier, res.args->last_usb_cookie);
    EXPECT_EQ(Status::SUCCESS, res.args->usb_last_status);

    if (!res.args->usb_last_port_status.supportsEnableContaminantPresenceDetection) return;

    for (int count = 1; count <= 10; count++) {
        currentStatus = !(res.args->usb_last_port_status.contaminantDetectionStatus ==
                          ContaminantDetectionStatus::DISABLED);

        ret = usb->enableContaminantPresenceDetection(
                res.args->usb_last_port_status.status_1_1.status.portName, !currentStatus);
        ASSERT_TRUE(ret.isOk());
        res = usb_cb_2->WaitForCallback(kCallbackNameNotifyPortStatusChange_1_2);
        EXPECT_TRUE(res.no_timeout);
        EXPECT_EQ(kCallbackIdentifier, res.args->last_usb_cookie);
        if (!currentStatus == !(res.args->usb_last_port_status.contaminantDetectionStatus ==
                                ContaminantDetectionStatus::DISABLED))
            successCount++;
    }

    if (!supported) EXPECT_GE(successCount, 9);
}

/*
 * When supportsEnableContaminantPresenceProtection is set false,
 * enableContaminantPresenceProtection should not enable/disable
 * contaminantPresenceProtection.
 */
TEST_P(UsbHidlTest, presenceProtectionSupportedCheck) {
    Return<void> ret = usb->queryPortStatus();
    ASSERT_TRUE(ret.isOk());
    auto res = usb_cb_2->WaitForCallback(kCallbackNameNotifyPortStatusChange_1_2);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(kCallbackIdentifier, res.args->last_usb_cookie);
    EXPECT_EQ(Status::SUCCESS, res.args->usb_last_status);

    if (!res.args->usb_last_port_status.supportsEnableContaminantPresenceProtection) {
        for (int runs = 1; runs <= 10; runs++) {
            bool currentStatus = !(res.args->usb_last_port_status.contaminantProtectionStatus ==
                                   ContaminantProtectionStatus::DISABLED);

            ret = usb->enableContaminantPresenceProtection(
                    res.args->usb_last_port_status.status_1_1.status.portName, !currentStatus);
            ASSERT_TRUE(ret.isOk());

            res = usb_cb_2->WaitForCallback(kCallbackNameNotifyPortStatusChange_1_2);
            EXPECT_TRUE(res.no_timeout);
            EXPECT_EQ(kCallbackIdentifier, res.args->last_usb_cookie);
            EXPECT_EQ(currentStatus, !(res.args->usb_last_port_status.contaminantProtectionStatus ==
                                       ContaminantProtectionStatus::DISABLED));
        }
    }
}

/*
 * enableContaminantPresenceProtection should succeed atleast 90% when supported.
 */
TEST_P(UsbHidlTest, contaminantPresenceProtectionStability) {
    int successCount = 0;
    bool currentStatus;
    bool supported = true;

    Return<void> ret = usb->queryPortStatus();
    ASSERT_TRUE(ret.isOk());
    auto res = usb_cb_2->WaitForCallback(kCallbackNameNotifyPortStatusChange_1_2);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(kCallbackIdentifier, res.args->last_usb_cookie);
    EXPECT_EQ(Status::SUCCESS, res.args->usb_last_status);

    if (!res.args->usb_last_port_status.supportsEnableContaminantPresenceProtection) return;

    for (int count = 1; count <= 10; count++) {
        currentStatus = !(res.args->usb_last_port_status.contaminantProtectionStatus ==
                          ContaminantProtectionStatus::DISABLED);

        ret = usb->enableContaminantPresenceProtection(
                res.args->usb_last_port_status.status_1_1.status.portName, !currentStatus);
        ASSERT_TRUE(ret.isOk());
        res = usb_cb_2->WaitForCallback(kCallbackNameNotifyPortStatusChange_1_2);
        EXPECT_TRUE(res.no_timeout);
        EXPECT_EQ(kCallbackIdentifier, res.args->last_usb_cookie);
        if (!currentStatus == !(res.args->usb_last_port_status.contaminantProtectionStatus ==
                                ContaminantProtectionStatus::DISABLED))
            successCount++;
    }

    if (!supported) EXPECT_GE(successCount, 9);
}

INSTANTIATE_TEST_SUITE_P(
        PerInstance, UsbHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IUsb::descriptor)),
        android::hardware::PrintInstanceNameToString);
