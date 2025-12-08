/*
 * Copyright 2025 The Android Open Source Project
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

#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_handler.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "aidl/android/hardware/bluetooth/ranging/BluetoothChannelSoundingParameters.h"
#include "aidl/android/hardware/bluetooth/ranging/BnBluetoothChannelSoundingSessionCallback.h"
#include "aidl/android/hardware/bluetooth/ranging/CsSecurityLevel.h"
#include "aidl/android/hardware/bluetooth/ranging/IBluetoothChannelSoundingSession.h"
#include "aidl/android/hardware/bluetooth/ranging/RangingResult.h"
#include "aidl/android/hardware/bluetooth/ranging/Reason.h"
#include "aidl/android/hardware/bluetooth/ranging/ResultType.h"
#include "aidl/android/hardware/bluetooth/ranging/SessionType.h"
#include "aidl/android/hardware/bluetooth/ranging/VendorSpecificData.h"
#include "android/binder_auto_utils.h"
#include "android/binder_interface_utils.h"
#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_util.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/test/mock/mock_android_base_wrapper.h"
#include "bluetooth_hal/test/mock/mock_cs_config_loader.h"
#include "bluetooth_hal/test/mock/mock_hci_router.h"
#include "bluetooth_hal/test/mock/mock_hci_router_client_agent.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace extensions {
namespace cs {
namespace {

using ::testing::_;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::Test;
using ::testing::Values;
using ::testing::WithParamInterface;

using ::aidl::android::hardware::bluetooth::ranging::
    BluetoothChannelSoundingParameters;
using ::aidl::android::hardware::bluetooth::ranging::
    BnBluetoothChannelSoundingSessionCallback;
using ::aidl::android::hardware::bluetooth::ranging::CsSecurityLevel;
using ::aidl::android::hardware::bluetooth::ranging::
    IBluetoothChannelSoundingSession;
using ::aidl::android::hardware::bluetooth::ranging::RangingResult;
using ::aidl::android::hardware::bluetooth::ranging::Reason;
using ::aidl::android::hardware::bluetooth::ranging::ResultType;
using ::aidl::android::hardware::bluetooth::ranging::SessionType;
using ::aidl::android::hardware::bluetooth::ranging::VendorSpecificData;

using ::bluetooth_hal::config::MockCsConfigLoader;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::MockHciRouter;
using ::bluetooth_hal::hci::MockHciRouterClientAgent;
using ::bluetooth_hal::util::MockAndroidBaseWrapper;

using ::ndk::ScopedAStatus;
using ::ndk::SharedRefBase;

constexpr uint16_t kDefaultAclHandle = 0x1234;

class MockBluetoothChannelSoundingSessionCallback
    : public BnBluetoothChannelSoundingSessionCallback {
 public:
  MOCK_METHOD(ScopedAStatus, onOpened, (Reason reason), (override));
  MOCK_METHOD(ScopedAStatus, onOpenFailed, (Reason reason), (override));
  MOCK_METHOD(ScopedAStatus, onResult, (const RangingResult& in_result),
              (override));
  MOCK_METHOD(ScopedAStatus, onClose, (Reason reason), (override));
  MOCK_METHOD(ScopedAStatus, onCloseFailed, (Reason reason), (override));
};

class TestBluetoothChannelSoundingHandler
    : public BluetoothChannelSoundingHandler {
 public:
  std::optional<std::reference_wrapper<SessionTracker>> GetTrackerWrapper(
      uint16_t connection_handle) {
    return GetTracker(connection_handle);
  }
};

class BluetoothChannelSoundingHandlerTest : public Test {
 protected:
  void SetUp() override {
    MockHciRouter::SetMockRouter(&mock_hci_router_);
    MockHciRouterClientAgent::SetMockAgent(&mock_hci_router_client_agent_);
    MockAndroidBaseWrapper::SetMockWrapper(&mock_android_base_wrapper_);
    MockCsConfigLoader::SetMockLoader(&mock_cs_config_loader_);

    bluetooth_channel_sounding_handler_ =
        std::make_unique<TestBluetoothChannelSoundingHandler>();
    mock_session_callback_ =
        SharedRefBase::make<MockBluetoothChannelSoundingSessionCallback>();
  }

  static BluetoothChannelSoundingParameters BuildParam(
      bool is_fake_notification_enabled, bool is_mode_0_channel_map_enabled) {
    BluetoothChannelSoundingParameters param;
    // Default acl handle.
    param.aclHandle = kDefaultAclHandle;
    param.vendorSpecificData = std::vector<std::optional<VendorSpecificData>>();
    param.vendorSpecificData->resize(2);
    param.vendorSpecificData->at(0) = VendorSpecificData{
        .characteristicUuid = kUuidSpecialRangingSettingCapability,
        .opaqueValue = std::vector<uint8_t>{
            0,
            static_cast<uint8_t>(is_fake_notification_enabled |
                                 (is_mode_0_channel_map_enabled) << 1),
            0, 0, 0}};
    param.vendorSpecificData->at(1) = VendorSpecificData{
        .characteristicUuid = kUuidSpecialRangingSettingCommand};

    return param;
  }

  static HalPacket BuildCsSubevent(uint16_t acl_handle,
                                   uint16_t procedure_counter) {
    return HalPacket({0x04, 0x3e, 0x09, kLeCsSubEventResultCode,
                      static_cast<uint8_t>(acl_handle & 0xff),
                      static_cast<uint8_t>((acl_handle >> 8) & 0xff), 0, 0, 0,
                      static_cast<uint8_t>(procedure_counter & 0xff), 0, 0});
  }

  static HalPacket BuildCsProcedureEnableCompleteEvent(uint16_t acl_handle) {
    return HalPacket({0x04, 0x3e, 0x09, kLeCsProcedureEnableCompleteCode, 0x00,
                      static_cast<uint8_t>(acl_handle & 0xff),
                      static_cast<uint8_t>((acl_handle >> 8) & 0xff), 0, 0});
  }

  std::shared_ptr<IBluetoothChannelSoundingSession> TestAndGetSession(
      const BluetoothChannelSoundingParameters& param,
      bool is_mode_0_channel_map_enabled, bool is_session_valid = true) {
    std::shared_ptr<IBluetoothChannelSoundingSession> session;

    HalPacket packet =
        BuildEnableMode0ChannelMapCommand(param.aclHandle, kCommandValueEnable);
    EXPECT_CALL(mock_hci_router_, SendCommand(packet, _))
        .Times(is_mode_0_channel_map_enabled);
    EXPECT_CALL(*mock_session_callback_, onOpened(Reason::LOCAL_STACK_REQUEST))
        .Times(1);
    EXPECT_TRUE(bluetooth_channel_sounding_handler_->OpenSession(
        param, mock_session_callback_, &session));
    EXPECT_EQ(session != nullptr, is_session_valid);

    return session;
  }

  std::optional<
      std::reference_wrapper<BluetoothChannelSoundingHandler::SessionTracker>>
  TestAndGetSessionTracker(uint16_t acl_handlle, uint16_t procedure_counter,
                           bool is_fake_notification_enabled) {
    auto session_tracker =
        bluetooth_channel_sounding_handler_->GetTrackerWrapper(acl_handlle);
    EXPECT_TRUE(session_tracker.has_value());
    EXPECT_EQ(session_tracker->get().cur_procedure_counter, procedure_counter);
    EXPECT_EQ(session_tracker->get().is_fake_notification_enabled,
              is_fake_notification_enabled);

    return session_tracker;
  }

  void TestHandleCsSubeventAndFakeNotification(
      uint16_t acl_handle, uint16_t cur_procedure_counter,
      uint16_t updated_procedure_counter, bool is_fake_notification_enabled,
      int expected_times) {
    auto session_tracker = TestAndGetSessionTracker(
        acl_handle, cur_procedure_counter, is_fake_notification_enabled);

    HalPacket cs_subevent =
        BuildCsSubevent(acl_handle, updated_procedure_counter);
    HalPacket ras_notification = BuildRasNotification(
        session_tracker->get().parameters, updated_procedure_counter);

    EXPECT_CALL(mock_hci_router_, SendPacketToStack(ras_notification))
        .Times(expected_times);
    bluetooth_channel_sounding_handler_->OnPacketCallback(cs_subevent);

    EXPECT_EQ(session_tracker->get().cur_procedure_counter,
              updated_procedure_counter);
    EXPECT_EQ(session_tracker->get().is_fake_notification_enabled,
              is_fake_notification_enabled);
  }

  std::unique_ptr<TestBluetoothChannelSoundingHandler>
      bluetooth_channel_sounding_handler_;
  MockHciRouter mock_hci_router_;
  MockHciRouterClientAgent mock_hci_router_client_agent_;
  MockAndroidBaseWrapper mock_android_base_wrapper_;
  MockCsConfigLoader mock_cs_config_loader_;
  std::shared_ptr<MockBluetoothChannelSoundingSessionCallback>
      mock_session_callback_;
};

TEST_F(BluetoothChannelSoundingHandlerTest, HandleCalibrationCommands) {
  const auto calibration_commands = std::vector<HalPacket>{
      HalPacket({0x01, 0x02, 0x03, 0x04}), HalPacket({0x01, 0x05, 0x06, 0x07})};
  EXPECT_CALL(mock_cs_config_loader_, GetCsCalibrationCommands)
      .Times(1)
      .WillOnce(ReturnRef(calibration_commands));
  EXPECT_CALL(mock_hci_router_, SendCommand(_, _)).Times(2);

  std::optional<std::vector<std::optional<VendorSpecificData>>> data;
  EXPECT_TRUE(
      bluetooth_channel_sounding_handler_->GetVendorSpecificData(&data));
}

TEST_F(BluetoothChannelSoundingHandlerTest, HandleEmptyCalibrationCommands) {
  std::vector<HalPacket> empty_calibration_commands;
  EXPECT_CALL(mock_cs_config_loader_, GetCsCalibrationCommands)
      .Times(1)
      .WillOnce(ReturnRef(empty_calibration_commands));
  EXPECT_CALL(mock_hci_router_, SendCommand(_, _)).Times(0);

  std::optional<std::vector<std::optional<VendorSpecificData>>> data;
  EXPECT_TRUE(
      bluetooth_channel_sounding_handler_->GetVendorSpecificData(&data));
}

TEST_F(BluetoothChannelSoundingHandlerTest, GetVendorSpecificDataReturnEmpty) {
  std::optional<std::vector<std::optional<VendorSpecificData>>> data;

  ON_CALL(mock_cs_config_loader_, GetCsCalibrationCommands)
      .WillByDefault([]() -> const std::vector<HalPacket>& {
        static const std::vector<HalPacket> kEmpty;
        return kEmpty;
      });

  EXPECT_TRUE(
      bluetooth_channel_sounding_handler_->GetVendorSpecificData(&data));
  EXPECT_FALSE(data.has_value());
}

TEST_F(BluetoothChannelSoundingHandlerTest,
       GetSupportedSessionTypesReturnDefaultValue) {
  std::optional<std::vector<SessionType>> session_types;

  EXPECT_TRUE(bluetooth_channel_sounding_handler_->GetSupportedSessionTypes(
      &session_types));
  EXPECT_TRUE(session_types.has_value());
  EXPECT_EQ(*session_types,
            std::vector<SessionType>{SessionType::SOFTWARE_STACK_DATA_PARSING});
}

TEST_F(BluetoothChannelSoundingHandlerTest,
       GetMaxSupportedCsSecurityLevelReturnDefaultValue) {
  CsSecurityLevel level;

  EXPECT_TRUE(
      bluetooth_channel_sounding_handler_->GetMaxSupportedCsSecurityLevel(
          &level));
  EXPECT_EQ(level, CsSecurityLevel::ONE);
}

TEST_F(BluetoothChannelSoundingHandlerTest,
       NotOpenSessionWithParamTypeVendorSpecificReply) {
  BluetoothChannelSoundingParameters param;

  // Build parameter for data reply type.
  param.vendorSpecificData = std::vector<std::optional<VendorSpecificData>>();
  param.vendorSpecificData->resize(2);
  param.vendorSpecificData->at(0) = VendorSpecificData{
      .characteristicUuid = kUuidSpecialRangingSettingCapability,
      .opaqueValue = {kDataTypeReply, 0, 0, 0, 0}};
  param.vendorSpecificData->at(1) = VendorSpecificData{
      .characteristicUuid = kUuidSpecialRangingSettingCommand};

  std::shared_ptr<IBluetoothChannelSoundingSession> session;

  EXPECT_CALL(mock_hci_router_, SendCommand(_, _)).Times(0);
  EXPECT_CALL(*mock_session_callback_, onOpened(_)).Times(0);
  EXPECT_TRUE(bluetooth_channel_sounding_handler_->OpenSession(
      param, mock_session_callback_, &session));
  EXPECT_EQ(session, nullptr);
}

// Parameterized test for fake notification and mode 0 channel map enablement.
struct OpenSessionTestParameters {
  bool is_fake_notification_enabled;
  bool is_mode_0_channel_map_enabled;
  bool expected_fake_notification_value;
};

class OpenSessionParameterizedTest
    : public BluetoothChannelSoundingHandlerTest,
      public WithParamInterface<OpenSessionTestParameters> {};

TEST_P(OpenSessionParameterizedTest, HandleNotificationAndMode0ChannelMap) {
  const auto& [is_fake_notification_enabled, is_mode_0_channel_map_enabled,
               expected_fake_notification_value] = GetParam();

  BluetoothChannelSoundingParameters param =
      BuildParam(is_fake_notification_enabled, is_mode_0_channel_map_enabled);

  TestAndGetSession(param, is_mode_0_channel_map_enabled);

  TestAndGetSessionTracker(kDefaultAclHandle, kInitialProcedureCounter,
                           expected_fake_notification_value);
}

INSTANTIATE_TEST_SUITE_P(
    CsSession, OpenSessionParameterizedTest,
    ::testing::Values(OpenSessionTestParameters{true, false, true},
                      OpenSessionTestParameters{false, false, false},
                      OpenSessionTestParameters{false, true, false},
                      OpenSessionTestParameters{true, true, true}));

TEST_F(BluetoothChannelSoundingHandlerTest,
       HandleCsSubEventButAclHandleNotMatched) {
  // CS subevent with acl handle kDefaultAclHandle and procedure counter
  // 0xffff.
  HalPacket cs_subevent =
      BuildCsSubevent(kDefaultAclHandle, kInitialProcedureCounter);

  bluetooth_channel_sounding_handler_->OnPacketCallback(cs_subevent);
  auto session_tracker =
      bluetooth_channel_sounding_handler_->GetTrackerWrapper(kDefaultAclHandle);
  EXPECT_FALSE(session_tracker.has_value());
}

TEST_F(BluetoothChannelSoundingHandlerTest,
       HandleCsSubEventAndSendFakeNotification) {
  bool is_fake_notification_enabled = true;
  bool is_mode_0_channel_map_enabled = false;
  BluetoothChannelSoundingParameters param =
      BuildParam(is_fake_notification_enabled, is_mode_0_channel_map_enabled);

  TestAndGetSession(param, is_mode_0_channel_map_enabled);

  uint16_t cur_procedure_counter = kInitialProcedureCounter;
  uint16_t updated_procedure_counter = 0x01;
  int expected_times = 1;
  TestHandleCsSubeventAndFakeNotification(
      kDefaultAclHandle, cur_procedure_counter, updated_procedure_counter,
      is_fake_notification_enabled, expected_times);
}

TEST_F(BluetoothChannelSoundingHandlerTest,
       HandleDuplicateCsSubEventAndNotSendSecondFakeNotification) {
  bool is_fake_notification_enabled = true;
  bool is_mode_0_channel_map_enabled = false;
  BluetoothChannelSoundingParameters param =
      BuildParam(is_fake_notification_enabled, is_mode_0_channel_map_enabled);

  TestAndGetSession(param, is_mode_0_channel_map_enabled);

  uint16_t cur_procedure_counter = kInitialProcedureCounter;
  uint16_t updated_procedure_counter = 0x01;
  int expected_times = 1;
  TestHandleCsSubeventAndFakeNotification(
      kDefaultAclHandle, cur_procedure_counter, updated_procedure_counter,
      is_fake_notification_enabled, expected_times);

  // Proceed the second same cs subevent and skip sending notification.
  cur_procedure_counter = 0x01;
  expected_times = 0;
  TestHandleCsSubeventAndFakeNotification(
      kDefaultAclHandle, cur_procedure_counter, updated_procedure_counter,
      is_fake_notification_enabled, expected_times);
}

TEST_F(BluetoothChannelSoundingHandlerTest,
       HandleCsSubEventButFakeNotificationNotEnabled) {
  bool is_fake_notification_enabled = false;
  bool is_mode_0_channel_map_enabled = false;
  BluetoothChannelSoundingParameters param =
      BuildParam(is_fake_notification_enabled, is_mode_0_channel_map_enabled);

  TestAndGetSession(param, is_mode_0_channel_map_enabled);

  uint16_t cur_procedure_counter = kInitialProcedureCounter;
  uint16_t updated_procedure_counter = kInitialProcedureCounter;
  int expected_times = 0;
  TestHandleCsSubeventAndFakeNotification(
      kDefaultAclHandle, cur_procedure_counter, updated_procedure_counter,
      is_fake_notification_enabled, expected_times);
}

TEST_F(BluetoothChannelSoundingHandlerTest,
       HandleCsProcedureEnableCompleteAndResetCounter) {
  bool is_fake_notification_enabled = true;
  bool is_mode_0_channel_map_enabled = true;
  BluetoothChannelSoundingParameters param =
      BuildParam(is_fake_notification_enabled, is_mode_0_channel_map_enabled);

  TestAndGetSession(param, is_mode_0_channel_map_enabled);

  uint16_t cur_procedure_counter = kInitialProcedureCounter;
  uint16_t updated_procedure_counter = 0x01;
  int expected_times = 1;
  TestHandleCsSubeventAndFakeNotification(
      kDefaultAclHandle, cur_procedure_counter, updated_procedure_counter,
      is_fake_notification_enabled, expected_times);

  // Subevent for cs procedure enable complete and reset counter from 0x01 to
  // 0xffff.
  HalPacket cs_procedure_complete_event =
      BuildCsProcedureEnableCompleteEvent(kDefaultAclHandle);

  bluetooth_channel_sounding_handler_->OnPacketCallback(
      cs_procedure_complete_event);
  // Counter is reset to 0xffff.
  auto session_tracker =
      bluetooth_channel_sounding_handler_->GetTrackerWrapper(kDefaultAclHandle);
  EXPECT_EQ(session_tracker->get().cur_procedure_counter,
            kInitialProcedureCounter);
}

TEST_F(BluetoothChannelSoundingHandlerTest,
       HandleCsProcedureEnableCompleteButNotResetCounter) {
  bool is_fake_notification_enabled = true;
  bool is_mode_0_channel_map_enabled = true;
  BluetoothChannelSoundingParameters param =
      BuildParam(is_fake_notification_enabled, is_mode_0_channel_map_enabled);

  TestAndGetSession(param, is_mode_0_channel_map_enabled);

  uint16_t cur_procedure_counter = kInitialProcedureCounter;
  uint16_t updated_procedure_counter = 0x01;
  int expected_times = 1;
  TestHandleCsSubeventAndFakeNotification(
      kDefaultAclHandle, cur_procedure_counter, updated_procedure_counter,
      is_fake_notification_enabled, expected_times);

  // Subevent for cs procedure enable complete for connection handle 0x1245.
  HalPacket cs_procedure_complete_event =
      BuildCsProcedureEnableCompleteEvent(0x1245);

  bluetooth_channel_sounding_handler_->OnPacketCallback(
      cs_procedure_complete_event);
  // Counter is not changed.
  auto session_tracker =
      bluetooth_channel_sounding_handler_->GetTrackerWrapper(kDefaultAclHandle);
  EXPECT_EQ(session_tracker->get().cur_procedure_counter,
            updated_procedure_counter);
}

class BluetoothChannelSoundingSessionTest
    : public BluetoothChannelSoundingHandlerTest {};

TEST_F(BluetoothChannelSoundingSessionTest,
       GetVendorSpecificRepliesWithUuidNotMatchedReturnEmpty) {
  BluetoothChannelSoundingParameters param;
  // Build parameter to not match with certain uuid.
  param.aclHandle = kDefaultAclHandle;

  bool is_mode_0_channel_map_enabled = false;
  std::shared_ptr<IBluetoothChannelSoundingSession> session =
      TestAndGetSession(param, is_mode_0_channel_map_enabled);

  std::optional<std::vector<std::optional<VendorSpecificData>>>
      vendor_specific_data;
  ScopedAStatus status =
      session->getVendorSpecificReplies(&vendor_specific_data);
  EXPECT_TRUE(status.isOk());
  EXPECT_FALSE(vendor_specific_data.has_value());
}

struct VendorSpecificTestParams {
  bool is_fake_notification_enabled;
  bool is_mode_0_channel_map_enabled;
  std::vector<std::optional<VendorSpecificData>> vendor_specific_data;
};

class VendorSpecificRepliesTest
    : public BluetoothChannelSoundingSessionTest,
      public WithParamInterface<VendorSpecificTestParams> {
 public:
  static std::vector<std::optional<VendorSpecificData>> BuildData(
      bool is_fake_notification_enabled, bool is_mode_0_channel_map_enabled) {
    uint8_t enable_inline_pct = is_fake_notification_enabled
                                    ? kCommandValueEnable
                                    : kCommandValueIgnore;
    uint8_t enable_mode_0_channel_map = is_mode_0_channel_map_enabled
                                            ? kCommandValueEnable
                                            : kCommandValueIgnore;

    VendorSpecificData capability;
    capability.characteristicUuid = kUuidSpecialRangingSettingCapability;
    capability.opaqueValue = {kDataTypeReply, 0x00, 0x00, 0x00, 0x00};

    VendorSpecificData command;
    command.characteristicUuid = kUuidSpecialRangingSettingCommand;
    command.opaqueValue = {
        kDataTypeReply,           enable_inline_pct, 0, 0, 0, 0,
        enable_mode_0_channel_map};

    return {capability, command};
  }
};

TEST_P(VendorSpecificRepliesTest, HandleDifferentVendorSpecificReplies) {
  const auto& [is_fake_notification_enabled, is_mode_0_channel_map_enabled,
               expected_vendor_specific_data] = GetParam();
  BluetoothChannelSoundingParameters param =
      BuildParam(is_fake_notification_enabled, is_mode_0_channel_map_enabled);

  std::shared_ptr<IBluetoothChannelSoundingSession> session =
      TestAndGetSession(param, is_mode_0_channel_map_enabled);

  std::optional<std::vector<std::optional<VendorSpecificData>>>
      vendor_specific_data;
  ScopedAStatus status =
      session->getVendorSpecificReplies(&vendor_specific_data);
  EXPECT_TRUE(status.isOk());
  EXPECT_TRUE(vendor_specific_data.has_value());
  EXPECT_EQ(*vendor_specific_data, expected_vendor_specific_data);
}

INSTANTIATE_TEST_SUITE_P(
    CsSession, VendorSpecificRepliesTest,
    Values(
        VendorSpecificTestParams{
            .is_fake_notification_enabled = false,
            .is_mode_0_channel_map_enabled = false,
            .vendor_specific_data =
                VendorSpecificRepliesTest::BuildData(false, false)},
        VendorSpecificTestParams{
            .is_fake_notification_enabled = false,
            .is_mode_0_channel_map_enabled = true,
            .vendor_specific_data = VendorSpecificRepliesTest::BuildData(false,
                                                                         true)},
        VendorSpecificTestParams{
            .is_fake_notification_enabled = true,
            .is_mode_0_channel_map_enabled = false,
            .vendor_specific_data =
                VendorSpecificRepliesTest::BuildData(true, false)},
        VendorSpecificTestParams{
            .is_fake_notification_enabled = true,
            .is_mode_0_channel_map_enabled = true,
            .vendor_specific_data =
                VendorSpecificRepliesTest::BuildData(true, true)}));

TEST_F(BluetoothChannelSoundingSessionTest,
       GetSupportedResultTypesReturnDefaultTypes) {
  bool is_fake_notification_enabled = true;
  bool is_mode_0_channel_map_enabled = false;
  BluetoothChannelSoundingParameters param =
      BuildParam(is_fake_notification_enabled, is_mode_0_channel_map_enabled);

  std::shared_ptr<IBluetoothChannelSoundingSession> session =
      TestAndGetSession(param, is_mode_0_channel_map_enabled);

  std::vector<ResultType> result_types;
  ScopedAStatus status = session->getSupportedResultTypes(&result_types);
  EXPECT_TRUE(status.isOk());
  EXPECT_EQ(result_types, std::vector<ResultType>{ResultType::RESULT_METERS});
}

TEST_F(BluetoothChannelSoundingSessionTest, HandleIsAbortedProcedureRequired) {
  bool is_fake_notification_enabled = true;
  bool is_mode_0_channel_map_enabled = false;
  BluetoothChannelSoundingParameters param =
      BuildParam(is_fake_notification_enabled, is_mode_0_channel_map_enabled);

  std::shared_ptr<IBluetoothChannelSoundingSession> session =
      TestAndGetSession(param, is_mode_0_channel_map_enabled);

  bool is_aborted_procedure_required = true;
  ScopedAStatus status =
      session->isAbortedProcedureRequired(&is_aborted_procedure_required);
  EXPECT_TRUE(status.isOk());
  EXPECT_FALSE(is_aborted_procedure_required);
}

TEST_F(BluetoothChannelSoundingSessionTest, CloseSession) {
  bool is_fake_notification_enabled = true;
  bool is_mode_0_channel_map_enabled = false;
  BluetoothChannelSoundingParameters param =
      BuildParam(is_fake_notification_enabled, is_mode_0_channel_map_enabled);

  std::shared_ptr<IBluetoothChannelSoundingSession> session =
      TestAndGetSession(param, is_mode_0_channel_map_enabled);

  EXPECT_CALL(*mock_session_callback_, onClose(Reason::LOCAL_STACK_REQUEST))
      .Times(1);
  ScopedAStatus status = session->close(Reason::LOCAL_STACK_REQUEST);
  EXPECT_TRUE(status.isOk());
}

}  // namespace
}  // namespace cs
}  // namespace extensions
}  // namespace bluetooth_hal
