/*
 * Copyright (C) 2021 The Android Open Source Probject
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

#define LOG_TAG "UsbAidlTest"
#include <android-base/logging.h>

#include <aidl/android/hardware/usb/IUsb.h>
#include <aidl/android/hardware/usb/IUsbCallback.h>
#include <aidl/android/hardware/usb/BnUsbCallback.h>
#include <aidl/android/hardware/usb/PortDataRole.h>
#include <aidl/android/hardware/usb/PortMode.h>
#include <aidl/android/hardware/usb/PortPowerRole.h>
#include <aidl/android/hardware/usb/PortRole.h>
#include <aidl/android/hardware/usb/PortStatus.h>
#include <aidl/android/hardware/usb/Status.h>
#include <aidl/Vintf.h>
#include <aidl/Gtest.h>

#include <android/binder_auto_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <gtest/gtest.h>

#include <log/log.h>
#include <stdlib.h>
#include <chrono>
#include <condition_variable>
#include <mutex>

#define TIMEOUT_PERIOD 10

using ::aidl::android::hardware::usb::AltModeData;
using ::aidl::android::hardware::usb::BnUsbCallback;
using ::aidl::android::hardware::usb::ComplianceWarning;
using ::aidl::android::hardware::usb::DisplayPortAltModePinAssignment;
using ::aidl::android::hardware::usb::DisplayPortAltModeStatus;
using ::aidl::android::hardware::usb::IUsb;
using ::aidl::android::hardware::usb::IUsbCallback;
using ::aidl::android::hardware::usb::LinkTrainingStatus;
using ::aidl::android::hardware::usb::PlugOrientation;
using ::aidl::android::hardware::usb::PortDataRole;
using ::aidl::android::hardware::usb::PortMode;
using ::aidl::android::hardware::usb::PortPowerRole;
using ::aidl::android::hardware::usb::PortRole;
using ::aidl::android::hardware::usb::PortStatus;
using ::aidl::android::hardware::usb::Status;
using ::aidl::android::hardware::usb::UsbDataStatus;

using ::ndk::ScopedAStatus;
using ::ndk::SpAIBinder;
using std::vector;
using std::shared_ptr;
using std::string;

// The main test class for the USB aidl hal
class UsbAidlTest : public testing::TestWithParam<std::string> {
 public:
  // Callback class for the USB aidl hal.
  // Usb Hal will call this object upon role switch or port query.
  class UsbCallback : public BnUsbCallback {
    UsbAidlTest& parent_;
    int cookie;

   public:
    UsbCallback(UsbAidlTest& parent, int cookie)
        : parent_(parent), cookie(cookie){};

    virtual ~UsbCallback() = default;

    // Callback method for the port status.
    ScopedAStatus notifyPortStatusChange(const vector<PortStatus>& currentPortStatus,
                                         Status retval) override {
      if (retval == Status::SUCCESS && currentPortStatus.size() > 0) {
        parent_.usb_last_port_status.portName =
            currentPortStatus[0].portName.c_str();
        parent_.usb_last_port_status.currentDataRole =
            currentPortStatus[0].currentDataRole;
        parent_.usb_last_port_status.currentPowerRole =
            currentPortStatus[0].currentPowerRole;
        parent_.usb_last_port_status.currentMode =
            currentPortStatus[0].currentMode;
      }
      parent_.usb_last_cookie = cookie;
      return ScopedAStatus::ok();
    }

    // Callback method for the status of role switch operation.
    ScopedAStatus notifyRoleSwitchStatus(const string& /*portName*/, const PortRole& newRole,
                                         Status retval, int64_t transactionId) override {
      parent_.usb_last_status = retval;
      parent_.usb_last_cookie = cookie;
      parent_.usb_last_port_role = newRole;
      parent_.usb_role_switch_done = true;
      parent_.last_transactionId = transactionId;
      parent_.notify();
      return ScopedAStatus::ok();
    }

    // Callback method for the status of enableUsbData operation
    ScopedAStatus notifyEnableUsbDataStatus(const string& /*portName*/, bool /*enable*/,
                                            Status /*retval*/, int64_t transactionId) override {
      parent_.last_transactionId = transactionId;
      parent_.usb_last_cookie = cookie;
      parent_.enable_usb_data_done = true;
      parent_.notify();
      return ScopedAStatus::ok();
    }

    // Callback method for the status of enableUsbData operation
    ScopedAStatus notifyEnableUsbDataWhileDockedStatus(const string& /*portName*/,
                                                       Status /*retval*/,
                                                       int64_t transactionId) override {
      parent_.last_transactionId = transactionId;
      parent_.usb_last_cookie = cookie;
      parent_.enable_usb_data_while_docked_done = true;
      parent_.notify();
      return ScopedAStatus::ok();
    }

    // Callback method for the status of enableContaminantPresenceDetection
    ScopedAStatus notifyContaminantEnabledStatus(const string& /*portName*/, bool /*enable*/,
                                                 Status /*retval*/, int64_t transactionId) override {
      parent_.last_transactionId = transactionId;
      parent_.usb_last_cookie = cookie;
      parent_.enable_contaminant_done = true;
      parent_.notify();
      return ScopedAStatus::ok();
    }

    // Callback method for the status of queryPortStatus operation
    ScopedAStatus notifyQueryPortStatus(const string& /*portName*/, Status /*retval*/,
                                        int64_t transactionId) override {
      parent_.last_transactionId = transactionId;
      parent_.notify();
      return ScopedAStatus::ok();
    }

    // Callback method for the status of limitPowerTransfer operation
    ScopedAStatus notifyLimitPowerTransferStatus(const string& /*portName*/, bool /*limit*/,
                                                 Status /*retval*/, int64_t transactionId) override {
      parent_.last_transactionId = transactionId;
      parent_.usb_last_cookie = cookie;
      parent_.limit_power_transfer_done = true;
      parent_.notify();
      return ScopedAStatus::ok();
    }

    // Callback method for the status of resetUsbPortStatus operation
    ScopedAStatus notifyResetUsbPortStatus(const string& /*portName*/, Status /*retval*/,
                                        int64_t transactionId) override {
      ALOGI("enter notifyResetUsbPortStatus");
      parent_.last_transactionId = transactionId;
      parent_.usb_last_cookie = cookie;
      parent_.reset_usb_port_done = true;
      parent_.notify();
      return ScopedAStatus::ok();
    }
  };

  virtual void SetUp() override {
    ALOGI("Setup");
    usb = IUsb::fromBinder(
                SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
    ASSERT_NE(usb, nullptr);

    usb_cb_2 = ::ndk::SharedRefBase::make<UsbCallback>(*this, 2);
    ASSERT_NE(usb_cb_2, nullptr);
    const auto& ret = usb->setCallback(usb_cb_2);
    ASSERT_TRUE(ret.isOk());
  }

  virtual void TearDown() override { ALOGI("Teardown"); }

  // Used as a mechanism to inform the test about data/event callback.
  inline void notify() {
    std::unique_lock<std::mutex> lock(usb_mtx);
    usb_count++;
    usb_cv.notify_one();
  }

  // Test code calls this function to wait for data/event callback.
  inline std::cv_status wait() {
    std::unique_lock<std::mutex> lock(usb_mtx);

    std::cv_status status = std::cv_status::no_timeout;
    auto now = std::chrono::system_clock::now();
    while (usb_count == 0) {
      status =
          usb_cv.wait_until(lock, now + std::chrono::seconds(TIMEOUT_PERIOD));
      if (status == std::cv_status::timeout) {
        ALOGI("timeout");
        return status;
      }
    }
    usb_count--;
    return status;
  }

  // USB aidl hal Proxy
  shared_ptr<IUsb> usb;

  // Callback objects for usb aidl
  // Methods of these objects are called to notify port status updates.
  shared_ptr<IUsbCallback> usb_cb_1, usb_cb_2;

  // The last conveyed status of the USB ports.
  // Stores information of currentt_data_role, power_role for all the USB ports
  PortStatus usb_last_port_status;

  // Status of the last role switch operation.
  Status usb_last_status;

  // Port role information of the last role switch operation.
  PortRole usb_last_port_role;

  // Flag to indicate the invocation of role switch callback.
  bool usb_role_switch_done;

  // Flag to indicate the invocation of notifyContaminantEnabledStatus callback.
  bool enable_contaminant_done;

  // Flag to indicate the invocation of notifyEnableUsbDataStatus callback.
  bool enable_usb_data_done;

  // Flag to indicate the invocation of notifyEnableUsbDataWhileDockedStatus callback.
  bool enable_usb_data_while_docked_done;

  // Flag to indicate the invocation of notifyLimitPowerTransferStatus callback.
  bool limit_power_transfer_done;

  // Flag to indicate the invocation of notifyResetUsbPort callback.
  bool reset_usb_port_done;

  // Stores the cookie of the last invoked usb callback object.
  int usb_last_cookie;

  // Last transaction ID that was recorded.
  int64_t last_transactionId;
  // synchronization primitives to coordinate between main test thread
  // and the callback thread.
  std::mutex usb_mtx;
  std::condition_variable usb_cv;
  int usb_count = 0;

  // Stores usb version
  int32_t usb_version;
};

/*
 * Test to see if setCallback succeeds.
 * Callback object is created and registered.
 */
TEST_P(UsbAidlTest, setCallback) {
  ALOGI("UsbAidlTest setCallback start");
  usb_cb_1 = ::ndk::SharedRefBase::make<UsbCallback>(*this, 1);
  ASSERT_NE(usb_cb_1, nullptr);
  const auto& ret = usb->setCallback(usb_cb_1);
  ASSERT_TRUE(ret.isOk());
  ALOGI("UsbAidlTest setCallback end");
}

/*
 * Check to see if querying type-c
 * port status succeeds.
 * The callback parameters are checked to see if the transaction id
 * matches.
 */
TEST_P(UsbAidlTest, queryPortStatus) {
  ALOGI("UsbAidlTest queryPortStatus start");
  int64_t transactionId = rand() % 10000;
  const auto& ret = usb->queryPortStatus(transactionId);
  ASSERT_TRUE(ret.isOk());
  EXPECT_EQ(std::cv_status::no_timeout, wait());
  EXPECT_EQ(2, usb_last_cookie);
  EXPECT_EQ(transactionId, last_transactionId);
  ALOGI("UsbAidlTest queryPortStatus end: %s", usb_last_port_status.portName.c_str());
}

/*
 * Query port status to Check to see whether only one of DISABLED_DOCK,
 * DISABLED_DOCK_DEVICE_MODE, DISABLED_DOCK_HOST_MODE is set at the most.
 * The callback parameters are checked to see if the transaction id
 * matches.
 */
TEST_P(UsbAidlTest, DisabledDataStatusCheck) {
  int disabledCount = 0;

  ALOGI("UsbAidlTest DataStatusCheck  start");
  auto retVersion = usb->getInterfaceVersion(&usb_version);
  ASSERT_TRUE(retVersion.isOk()) << retVersion;
  if (usb_version < 2) {
    ALOGI("UsbAidlTest skipping DataStatusCheck on older interface versions");
    GTEST_SKIP();
  }
  int64_t transactionId = rand() % 10000;
  const auto& ret = usb->queryPortStatus(transactionId);
  ASSERT_TRUE(ret.isOk());
  EXPECT_EQ(std::cv_status::no_timeout, wait());
  EXPECT_EQ(2, usb_last_cookie);
  EXPECT_EQ(transactionId, last_transactionId);
  ALOGI("UsbAidlTest DataStatusCheck portName: %s", usb_last_port_status.portName.c_str());
  if (usb_last_port_status.usbDataStatus.size() > 1) {
    for (UsbDataStatus dataStatus : usb_last_port_status.usbDataStatus) {
      if (dataStatus == UsbDataStatus::DISABLED_DOCK ||
          dataStatus == UsbDataStatus::DISABLED_DOCK_DEVICE_MODE ||
          dataStatus == UsbDataStatus::DISABLED_DOCK_HOST_MODE) {
        disabledCount++;
      }
    }
  }
  EXPECT_GE(1, disabledCount);
  ALOGI("UsbAidlTest DataStatusCheck end");
}

/*
 * Trying to switch a non-existent port should fail.
 * This test case tried to switch the port with empty
 * name which is expected to fail.
 * The callback parameters are checked to see if the transaction id
 * matches.
 */
TEST_P(UsbAidlTest, switchEmptyPort) {
  ALOGI("UsbAidlTest switchEmptyPort start");
  PortRole role;
  role.set<PortRole::powerRole>(PortPowerRole::SOURCE);
  int64_t transactionId = rand() % 10000;
  const auto& ret = usb->switchRole("", role, transactionId);
  ASSERT_TRUE(ret.isOk());
  EXPECT_EQ(std::cv_status::no_timeout, wait());
  EXPECT_EQ(Status::ERROR, usb_last_status);
  EXPECT_EQ(transactionId, last_transactionId);
  EXPECT_EQ(2, usb_last_cookie);
  ALOGI("UsbAidlTest switchEmptyPort end");
}

/*
 * Test switching the power role of usb port.
 * Test case queries the usb ports present in device.
 * If there is at least one usb port, a power role switch
 * to SOURCE is attempted for the port.
 * The callback parameters are checked to see if the transaction id
 * matches.
 */
TEST_P(UsbAidlTest, switchPowerRole) {
  ALOGI("UsbAidlTest switchPowerRole start");
  PortRole role;
  role.set<PortRole::powerRole>(PortPowerRole::SOURCE);
  int64_t transactionId = rand() % 10000;
  const auto& ret = usb->queryPortStatus(transactionId);
  ASSERT_TRUE(ret.isOk());
  EXPECT_EQ(std::cv_status::no_timeout, wait());
  EXPECT_EQ(2, usb_last_cookie);
  EXPECT_EQ(transactionId, last_transactionId);

  if (!usb_last_port_status.portName.empty()) {
    string portBeingSwitched = usb_last_port_status.portName;
    ALOGI("switchPower role portname:%s", portBeingSwitched.c_str());
    usb_role_switch_done = false;
    transactionId = rand() % 10000;
    const auto& ret = usb->switchRole(portBeingSwitched, role, transactionId);
    ASSERT_TRUE(ret.isOk());

    std::cv_status waitStatus = wait();
    while (waitStatus == std::cv_status::no_timeout &&
           usb_role_switch_done == false)
      waitStatus = wait();

    EXPECT_EQ(std::cv_status::no_timeout, waitStatus);
    EXPECT_EQ(2, usb_last_cookie);
    EXPECT_EQ(transactionId, last_transactionId);
  }
  ALOGI("UsbAidlTest switchPowerRole end");
}

/*
 * Test switching the data role of usb port.
 * Test case queries the usb ports present in device.
 * If there is at least one usb port, a data role switch
 * to device is attempted for the port.
 * The callback parameters are checked to see if transaction id
 * matches.
 */
TEST_P(UsbAidlTest, switchDataRole) {
  ALOGI("UsbAidlTest switchDataRole start");
  PortRole role;
  role.set<PortRole::dataRole>(PortDataRole::DEVICE);
  int64_t transactionId = rand() % 10000;
  const auto& ret = usb->queryPortStatus(transactionId);
  ASSERT_TRUE(ret.isOk());
  EXPECT_EQ(std::cv_status::no_timeout, wait());
  EXPECT_EQ(2, usb_last_cookie);
  EXPECT_EQ(transactionId, last_transactionId);

  if (!usb_last_port_status.portName.empty()) {
    string portBeingSwitched = usb_last_port_status.portName;
    ALOGI("portname:%s", portBeingSwitched.c_str());
    usb_role_switch_done = false;
    transactionId = rand() % 10000;
    const auto& ret = usb->switchRole(portBeingSwitched, role, transactionId);
    ASSERT_TRUE(ret.isOk());

    std::cv_status waitStatus = wait();
    while (waitStatus == std::cv_status::no_timeout &&
           usb_role_switch_done == false)
      waitStatus = wait();

    EXPECT_EQ(std::cv_status::no_timeout, waitStatus);
    EXPECT_EQ(2, usb_last_cookie);
    EXPECT_EQ(transactionId, last_transactionId);
  }
  ALOGI("UsbAidlTest switchDataRole end");
}

/*
 * Test enabling contaminant presence detection of the port.
 * Test case queries the usb ports present in device.
 * If there is at least one usb port, enabling contaminant detection
 * is attempted for the port.
 * The callback parameters are checked to see if transaction id
 * matches.
 */
TEST_P(UsbAidlTest, enableContaminantPresenceDetection) {
  ALOGI("UsbAidlTest enableContaminantPresenceDetection start");
  int64_t transactionId = rand() % 10000;
  const auto& ret = usb->queryPortStatus(transactionId);
  ASSERT_TRUE(ret.isOk());
  EXPECT_EQ(std::cv_status::no_timeout, wait());
  EXPECT_EQ(2, usb_last_cookie);
  EXPECT_EQ(transactionId, last_transactionId);

  if (!usb_last_port_status.portName.empty()) {
    ALOGI("portname:%s", usb_last_port_status.portName.c_str());
    enable_contaminant_done = false;
    transactionId = rand() % 10000;
    const auto& ret = usb->enableContaminantPresenceDetection(usb_last_port_status.portName,
                                                              true, transactionId);
    ASSERT_TRUE(ret.isOk());

    std::cv_status waitStatus = wait();
    while (waitStatus == std::cv_status::no_timeout &&
           enable_contaminant_done == false)
      waitStatus = wait();

    EXPECT_EQ(std::cv_status::no_timeout, waitStatus);
    EXPECT_EQ(2, usb_last_cookie);
    EXPECT_EQ(transactionId, last_transactionId);
  }
  ALOGI("UsbAidlTest enableContaminantPresenceDetection end");
}

/*
 * Test enabling Usb data of the port.
 * Test case queries the usb ports present in device.
 * If there is at least one usb port, enabling Usb data is attempted
 * for the port.
 * The callback parameters are checked to see if transaction id
 * matches.
 */
TEST_P(UsbAidlTest, enableUsbData) {
  ALOGI("UsbAidlTest enableUsbData start");
  int64_t transactionId = rand() % 10000;
  const auto& ret = usb->queryPortStatus(transactionId);
  ASSERT_TRUE(ret.isOk());
  EXPECT_EQ(std::cv_status::no_timeout, wait());
  EXPECT_EQ(2, usb_last_cookie);
  EXPECT_EQ(transactionId, last_transactionId);

  if (!usb_last_port_status.portName.empty()) {
    ALOGI("portname:%s", usb_last_port_status.portName.c_str());
    enable_usb_data_done = false;
    transactionId = rand() % 10000;
    const auto& ret = usb->enableUsbData(usb_last_port_status.portName, true, transactionId);
    ASSERT_TRUE(ret.isOk());

    std::cv_status waitStatus = wait();
    while (waitStatus == std::cv_status::no_timeout &&
           enable_usb_data_done == false)
      waitStatus = wait();

    EXPECT_EQ(std::cv_status::no_timeout, waitStatus);
    EXPECT_EQ(2, usb_last_cookie);
    EXPECT_EQ(transactionId, last_transactionId);
  }
  ALOGI("UsbAidlTest enableUsbData end");
}

/*
 * Test enabling Usb data while being docked.
 * Test case queries the usb ports present in device.
 * If there is at least one usb port, enabling Usb data while docked
 * is attempted for the port.
 * The callback parameters are checked to see if transaction id
 * matches.
 */
TEST_P(UsbAidlTest, enableUsbDataWhileDocked) {
  ALOGI("UsbAidlTest enableUsbDataWhileDocked start");
  int64_t transactionId = rand() % 10000;
  const auto& ret = usb->queryPortStatus(transactionId);
  ASSERT_TRUE(ret.isOk());
  EXPECT_EQ(std::cv_status::no_timeout, wait());
  EXPECT_EQ(2, usb_last_cookie);
  EXPECT_EQ(transactionId, last_transactionId);

  if (!usb_last_port_status.portName.empty()) {
    ALOGI("portname:%s", usb_last_port_status.portName.c_str());
    enable_usb_data_while_docked_done = false;
    transactionId = rand() % 10000;
    const auto& ret = usb->enableUsbDataWhileDocked(usb_last_port_status.portName, transactionId);
    ASSERT_TRUE(ret.isOk());

    std::cv_status waitStatus = wait();
    while (waitStatus == std::cv_status::no_timeout &&
           enable_usb_data_while_docked_done == false)
      waitStatus = wait();

    EXPECT_EQ(std::cv_status::no_timeout, waitStatus);
    EXPECT_EQ(2, usb_last_cookie);
    EXPECT_EQ(transactionId, last_transactionId);
  }
  ALOGI("UsbAidlTest enableUsbDataWhileDocked end");
}

/*
 * Test enabling Usb data of the port.
 * Test case queries the usb ports present in device.
 * If there is at least one usb port, relaxing limit power transfer
 * is attempted for the port.
 * The callback parameters are checked to see if transaction id
 * matches.
 */
TEST_P(UsbAidlTest, limitPowerTransfer) {
  ALOGI("UsbAidlTest limitPowerTransfer start");
  int64_t transactionId = rand() % 10000;
  const auto& ret = usb->queryPortStatus(transactionId);
  ASSERT_TRUE(ret.isOk());
  EXPECT_EQ(std::cv_status::no_timeout, wait());
  EXPECT_EQ(2, usb_last_cookie);
  EXPECT_EQ(transactionId, last_transactionId);

  if (!usb_last_port_status.portName.empty()) {
    ALOGI("portname:%s", usb_last_port_status.portName.c_str());
    limit_power_transfer_done = false;
    transactionId = rand() % 10000;
    const auto& ret = usb->limitPowerTransfer(usb_last_port_status.portName, false, transactionId);
    ASSERT_TRUE(ret.isOk());

    std::cv_status waitStatus = wait();
    while (waitStatus == std::cv_status::no_timeout &&
           limit_power_transfer_done == false)
      waitStatus = wait();

    EXPECT_EQ(std::cv_status::no_timeout, waitStatus);
    EXPECT_EQ(2, usb_last_cookie);
    EXPECT_EQ(transactionId, last_transactionId);
  }
  ALOGI("UsbAidlTest limitPowerTransfer end");
}

/*
 * Test reset Usb data of the port.
 * Test case queries the usb ports present in device.
 * If there is at least one usb port, reset Usb data for the port.
 * The callback parameters are checked to see if transaction id
 * matches.
 */
TEST_P(UsbAidlTest, DISABLED_resetUsbPort) {
  ALOGI("UsbAidlTest resetUsbPort start");
  int64_t transactionId = rand() % 10000;
  const auto& ret = usb->queryPortStatus(transactionId);
  ASSERT_TRUE(ret.isOk());
  EXPECT_EQ(std::cv_status::no_timeout, wait());
  EXPECT_EQ(2, usb_last_cookie);
  EXPECT_EQ(transactionId, last_transactionId);

  if (!usb_last_port_status.portName.empty()) {
    ALOGI("portname:%s", usb_last_port_status.portName.c_str());
    reset_usb_port_done = false;
    transactionId = rand() % 10000;
    const auto& ret = usb->resetUsbPort(usb_last_port_status.portName, transactionId);
    ASSERT_TRUE(ret.isOk());
    ALOGI("UsbAidlTest resetUsbPort ret.isOk");

    std::cv_status waitStatus = wait();
    while (waitStatus == std::cv_status::no_timeout &&
           reset_usb_port_done == false)
      waitStatus = wait();

    ALOGI("UsbAidlTest resetUsbPort wait()");
    EXPECT_EQ(std::cv_status::no_timeout, waitStatus);
    EXPECT_EQ(2, usb_last_cookie);
    EXPECT_EQ(transactionId, last_transactionId);
  }
  ALOGI("UsbAidlTest resetUsbPort end");
}

/*
 * Test charger compliance warning
 * The test asserts that complianceWarnings is
 * empty when the feature is not supported. i.e.
 * supportsComplianceWarning is false.
 */
TEST_P(UsbAidlTest, nonCompliantChargerStatus) {
  ALOGI("UsbAidlTest nonCompliantChargerStatus start");
  auto retVersion = usb->getInterfaceVersion(&usb_version);
  ASSERT_TRUE(retVersion.isOk()) << retVersion;
  if (usb_version < 2) {
    ALOGI("UsbAidlTest skipping nonCompliantChargerStatus on older interface versions");
    GTEST_SKIP();
  }
  int64_t transactionId = rand() % 10000;
  const auto& ret = usb->queryPortStatus(transactionId);
  ASSERT_TRUE(ret.isOk());
  EXPECT_EQ(std::cv_status::no_timeout, wait());
  EXPECT_EQ(2, usb_last_cookie);
  EXPECT_EQ(transactionId, last_transactionId);

  if (!usb_last_port_status.supportsComplianceWarnings) {
    EXPECT_TRUE(usb_last_port_status.complianceWarnings.empty());
  }

  ALOGI("UsbAidlTest nonCompliantChargerStatus end");
}

/*
 * Test charger compliance warning values
 * The test asserts that complianceWarning values
 * are valid.
 */
TEST_P(UsbAidlTest, nonCompliantChargerValues) {
  ALOGI("UsbAidlTest nonCompliantChargerValues start");
  auto retVersion = usb->getInterfaceVersion(&usb_version);
  ASSERT_TRUE(retVersion.isOk()) << retVersion;
  if (usb_version < 2) {
    ALOGI("UsbAidlTest skipping nonCompliantChargerValues on older interface versions");
    GTEST_SKIP();
  }
  int64_t transactionId = rand() % 10000;
  const auto& ret = usb->queryPortStatus(transactionId);
  ASSERT_TRUE(ret.isOk());
  EXPECT_EQ(std::cv_status::no_timeout, wait());
  EXPECT_EQ(2, usb_last_cookie);
  EXPECT_EQ(transactionId, last_transactionId);

  if (usb_last_port_status.supportsComplianceWarnings) {
    for (auto warning : usb_last_port_status.complianceWarnings) {
      EXPECT_TRUE((int)warning >= (int)ComplianceWarning::OTHER);
      /*
       * Version 2 compliance values range from [1, 4]
       * Version 3 compliance values range from [1, 9]
       */
      if (usb_version < 3) {
        EXPECT_TRUE((int)warning <= (int)ComplianceWarning::MISSING_RP);
      } else {
        EXPECT_TRUE((int)warning <= (int)ComplianceWarning::UNRELIABLE_IO);
      }
    }
  }

  ALOGI("UsbAidlTest nonCompliantChargerValues end");
}

/*
 * Test PlugOrientation Values are within range in PortStatus
 */
TEST_P(UsbAidlTest, plugOrientationValues) {
  ALOGI("UsbAidlTest plugOrientationValues start");
  auto retVersion = usb->getInterfaceVersion(&usb_version);
  ASSERT_TRUE(retVersion.isOk()) << retVersion;
  if (usb_version < 2) {
    ALOGI("UsbAidlTest skipping plugOrientationValues on older interface versions");
    GTEST_SKIP();
  }
  int64_t transactionId = rand() % 10000;
  const auto& ret = usb->queryPortStatus(transactionId);
  ASSERT_TRUE(ret.isOk());
  EXPECT_EQ(std::cv_status::no_timeout, wait());
  EXPECT_EQ(2, usb_last_cookie);
  EXPECT_EQ(transactionId, last_transactionId);

  EXPECT_TRUE((int)usb_last_port_status.plugOrientation >= (int)PlugOrientation::UNKNOWN);
  EXPECT_TRUE((int)usb_last_port_status.plugOrientation <= (int)PlugOrientation::PLUGGED_FLIPPED);
}

/*
 * Test DisplayPortAltMode Values when DisplayPort Alt Mode
 * is active.
 */
TEST_P(UsbAidlTest, dpAltModeValues) {
  ALOGI("UsbAidlTest dpAltModeValues start");
  auto retVersion = usb->getInterfaceVersion(&usb_version);
  ASSERT_TRUE(retVersion.isOk()) << retVersion;
  if (usb_version < 2) {
    ALOGI("UsbAidlTest skipping dpAltModeValues on older interface versions");
    GTEST_SKIP();
  }
  int64_t transactionId = rand() % 10000;
  const auto& ret = usb->queryPortStatus(transactionId);
  ASSERT_TRUE(ret.isOk());
  EXPECT_EQ(std::cv_status::no_timeout, wait());
  EXPECT_EQ(2, usb_last_cookie);
  EXPECT_EQ(transactionId, last_transactionId);

  // Discover DisplayPort Alt Mode
  for (AltModeData altMode : usb_last_port_status.supportedAltModes) {
    if (altMode.getTag() == AltModeData::displayPortAltModeData) {
      AltModeData::DisplayPortAltModeData displayPortAltModeData =
              altMode.get<AltModeData::displayPortAltModeData>();
      EXPECT_TRUE((int)displayPortAltModeData.partnerSinkStatus >=
                  (int)DisplayPortAltModeStatus::UNKNOWN);
      EXPECT_TRUE((int)displayPortAltModeData.partnerSinkStatus <=
                  (int)DisplayPortAltModeStatus::ENABLED);

      EXPECT_TRUE((int)displayPortAltModeData.cableStatus >=
                  (int)DisplayPortAltModeStatus::UNKNOWN);
      EXPECT_TRUE((int)displayPortAltModeData.cableStatus <=
                  (int)DisplayPortAltModeStatus::ENABLED);

      EXPECT_TRUE((int)displayPortAltModeData.pinAssignment >=
                  (int)DisplayPortAltModePinAssignment::NONE);
      EXPECT_TRUE((int)displayPortAltModeData.pinAssignment <=
                  (int)DisplayPortAltModePinAssignment::F);

      EXPECT_TRUE((int)displayPortAltModeData.linkTrainingStatus >=
                  (int)LinkTrainingStatus::UNKNOWN);
      EXPECT_TRUE((int)displayPortAltModeData.pinAssignment <= (int)LinkTrainingStatus::FAILURE);
    }
  }

  ALOGI("UsbAidlTest dpAltModeValues end");
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(UsbAidlTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, UsbAidlTest,
        testing::ValuesIn(::android::getAidlHalInstanceNames(IUsb::descriptor)),
        ::android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
