/*
 * Copyright (C) 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/bluetooth/ranging/BnBluetoothChannelSoundingSessionCallback.h>
#include <aidl/android/hardware/bluetooth/ranging/IBluetoothChannelSounding.h>
#include <aidl/android/hardware/bluetooth/ranging/IBluetoothChannelSoundingSessionCallback.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <binder/IServiceManager.h>
#include <utils/Log.h>

using aidl::android::hardware::bluetooth::ranging::
    BluetoothChannelSoundingParameters;
using aidl::android::hardware::bluetooth::ranging::
    BnBluetoothChannelSoundingSessionCallback;
using aidl::android::hardware::bluetooth::ranging::ChannelSoudingRawData;
using aidl::android::hardware::bluetooth::ranging::CsSecurityLevel;
using aidl::android::hardware::bluetooth::ranging::IBluetoothChannelSounding;
using aidl::android::hardware::bluetooth::ranging::
    IBluetoothChannelSoundingSession;
using aidl::android::hardware::bluetooth::ranging::
    IBluetoothChannelSoundingSessionCallback;
using aidl::android::hardware::bluetooth::ranging::RangingResult;
using aidl::android::hardware::bluetooth::ranging::Reason;
using aidl::android::hardware::bluetooth::ranging::ResultType;
using aidl::android::hardware::bluetooth::ranging::SessionType;
using aidl::android::hardware::bluetooth::ranging::VendorSpecificData;
using ndk::ScopedAStatus;

class BluetoothChannelSoundingSessionCallback
    : public BnBluetoothChannelSoundingSessionCallback {
 public:
  ScopedAStatus onOpened(Reason reason) override;
  ScopedAStatus onOpenFailed(Reason reason) override;
  ScopedAStatus onResult(const RangingResult& in_result) override;
  ScopedAStatus onClose(Reason reason) override;
  ScopedAStatus onCloseFailed(Reason reason) override;
};

ScopedAStatus BluetoothChannelSoundingSessionCallback::onOpened(
    Reason /*reason*/) {
  return ::ndk::ScopedAStatus::ok();
}
ScopedAStatus BluetoothChannelSoundingSessionCallback::onOpenFailed(
    Reason /*reason*/) {
  return ::ndk::ScopedAStatus::ok();
}
ScopedAStatus BluetoothChannelSoundingSessionCallback::onResult(
    const RangingResult& /*in_result*/) {
  return ::ndk::ScopedAStatus::ok();
}
ScopedAStatus BluetoothChannelSoundingSessionCallback::onClose(
    Reason /*reason*/) {
  return ::ndk::ScopedAStatus::ok();
}
ScopedAStatus BluetoothChannelSoundingSessionCallback::onCloseFailed(
    Reason /*reason*/) {
  return ::ndk::ScopedAStatus::ok();
}

class BluetoothRangingTest : public ::testing::TestWithParam<std::string> {
 public:
  virtual void SetUp() override {
    ALOGI("SetUp Ranging Test");
    bluetooth_channel_sounding_ = IBluetoothChannelSounding::fromBinder(
        ndk::SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
    ASSERT_NE(bluetooth_channel_sounding_, nullptr);
  }

  virtual void TearDown() override {
    ALOGI("TearDown Ranging Test");
    bluetooth_channel_sounding_ = nullptr;
    ASSERT_EQ(bluetooth_channel_sounding_, nullptr);
  }

  ScopedAStatus getVendorSpecificData(
      std::optional<std::vector<std::optional<VendorSpecificData>>>*
          _aidl_return);
  ScopedAStatus getSupportedSessionTypes(
      std::optional<std::vector<SessionType>>* _aidl_return);
  ScopedAStatus getMaxSupportedCsSecurityLevel(CsSecurityLevel* _aidl_return);
  ScopedAStatus openSession(
      const BluetoothChannelSoundingParameters& in_params,
      const std::shared_ptr<IBluetoothChannelSoundingSessionCallback>&
          in_callback,
      std::shared_ptr<IBluetoothChannelSoundingSession>* _aidl_return);

  ScopedAStatus initBluetoothChannelSoundingSession(
      std::shared_ptr<IBluetoothChannelSoundingSession>* session) {
    BluetoothChannelSoundingParameters params;
    std::shared_ptr<BluetoothChannelSoundingSessionCallback> callback = nullptr;
    callback =
        ndk::SharedRefBase::make<BluetoothChannelSoundingSessionCallback>();
    ScopedAStatus status = openSession(params, callback, session);
    return status;
  }

 private:
  std::shared_ptr<IBluetoothChannelSounding> bluetooth_channel_sounding_;
};

ScopedAStatus BluetoothRangingTest::getVendorSpecificData(
    std::optional<std::vector<std::optional<VendorSpecificData>>>*
        _aidl_return) {
  return bluetooth_channel_sounding_->getVendorSpecificData(_aidl_return);
}
ScopedAStatus BluetoothRangingTest::getSupportedSessionTypes(
    std::optional<std::vector<SessionType>>* _aidl_return) {
  return bluetooth_channel_sounding_->getSupportedSessionTypes(_aidl_return);
}

ScopedAStatus BluetoothRangingTest::getMaxSupportedCsSecurityLevel(
    CsSecurityLevel* _aidl_return) {
  return bluetooth_channel_sounding_->getMaxSupportedCsSecurityLevel(
      _aidl_return);
}
ScopedAStatus BluetoothRangingTest::openSession(
    const BluetoothChannelSoundingParameters& in_params,
    const std::shared_ptr<IBluetoothChannelSoundingSessionCallback>&
        in_callback,
    std::shared_ptr<IBluetoothChannelSoundingSession>* _aidl_return) {
  return bluetooth_channel_sounding_->openSession(in_params, in_callback,
                                                  _aidl_return);
}

TEST_P(BluetoothRangingTest, SetupAndTearDown) {}

TEST_P(BluetoothRangingTest, GetVendorSpecificData) {
  std::optional<std::vector<std::optional<VendorSpecificData>>>
      vendor_specific_data;
  ScopedAStatus status = getVendorSpecificData(&vendor_specific_data);
  ASSERT_TRUE(status.isOk());
}

TEST_P(BluetoothRangingTest, GetSupportedSessionTypes) {
  std::optional<std::vector<SessionType>> supported_session_types;
  ScopedAStatus status = getSupportedSessionTypes(&supported_session_types);
  ASSERT_TRUE(status.isOk());
}

TEST_P(BluetoothRangingTest, GetMaxSupportedCsSecurityLevel) {
  CsSecurityLevel security_level;
  ScopedAStatus status = getMaxSupportedCsSecurityLevel(&security_level);
  ASSERT_TRUE(status.isOk());
}

TEST_P(BluetoothRangingTest, OpenSession) {
  BluetoothChannelSoundingParameters params;
  std::shared_ptr<BluetoothChannelSoundingSessionCallback> callback = nullptr;
  callback =
      ndk::SharedRefBase::make<BluetoothChannelSoundingSessionCallback>();
  std::shared_ptr<IBluetoothChannelSoundingSession> session;
  ScopedAStatus status = openSession(params, callback, &session);
  ASSERT_TRUE(status.isOk());
}

TEST_P(BluetoothRangingTest, GetVendorSpecificReplies) {
  std::shared_ptr<IBluetoothChannelSoundingSession> session;
  auto status = initBluetoothChannelSoundingSession(&session);
  ASSERT_TRUE(status.isOk());
  if (session != nullptr) {
    std::optional<std::vector<std::optional<VendorSpecificData>>>
        vendor_specific_data;
    status = session->getVendorSpecificReplies(&vendor_specific_data);
    ASSERT_TRUE(status.isOk());
  }
}

TEST_P(BluetoothRangingTest, GetSupportedResultTypes) {
  std::shared_ptr<IBluetoothChannelSoundingSession> session;
  auto status = initBluetoothChannelSoundingSession(&session);
  ASSERT_TRUE(status.isOk());
  if (session != nullptr) {
    std::vector<ResultType> supported_result_types;
    status = session->getSupportedResultTypes(&supported_result_types);
    ASSERT_TRUE(status.isOk());
  }
}

TEST_P(BluetoothRangingTest, IsAbortedProcedureRequired) {
  std::shared_ptr<IBluetoothChannelSoundingSession> session;
  auto status = initBluetoothChannelSoundingSession(&session);
  ASSERT_TRUE(status.isOk());
  if (session != nullptr) {
    bool is_abort_procedure_required = true;
    status = session->isAbortedProcedureRequired(&is_abort_procedure_required);
    ASSERT_TRUE(status.isOk());
  }
}

TEST_P(BluetoothRangingTest, WriteRawData) {
  std::shared_ptr<IBluetoothChannelSoundingSession> session;
  auto status = initBluetoothChannelSoundingSession(&session);
  ASSERT_TRUE(status.isOk());
  if (session != nullptr) {
    ChannelSoudingRawData raw_data;
    status = session->writeRawData(raw_data);
    ASSERT_TRUE(status.isOk());
  }
}

TEST_P(BluetoothRangingTest, CloseSession) {
  std::shared_ptr<IBluetoothChannelSoundingSession> session;
  auto status = initBluetoothChannelSoundingSession(&session);
  ASSERT_TRUE(status.isOk());
  if (session != nullptr) {
    status = session->close(Reason::LOCAL_STACK_REQUEST);
    ASSERT_TRUE(status.isOk());
  }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(BluetoothRangingTest);
INSTANTIATE_TEST_SUITE_P(PerInstance, BluetoothRangingTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(
                             IBluetoothChannelSounding::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ABinderProcess_startThreadPool();
  int status = RUN_ALL_TESTS();
  ALOGI("Test result = %d", status);
  return status;
}