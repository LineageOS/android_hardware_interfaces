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

#define LOG_TAG "bthal.extensions.cs"

#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_handler.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "aidl/android/hardware/bluetooth/ranging/BluetoothChannelSoundingParameters.h"
#include "aidl/android/hardware/bluetooth/ranging/CsSecurityLevel.h"
#include "aidl/android/hardware/bluetooth/ranging/IBluetoothChannelSoundingSession.h"
#include "aidl/android/hardware/bluetooth/ranging/IBluetoothChannelSoundingSessionCallback.h"
#include "aidl/android/hardware/bluetooth/ranging/Reason.h"
#include "aidl/android/hardware/bluetooth/ranging/SessionType.h"
#include "aidl/android/hardware/bluetooth/ranging/VendorSpecificData.h"
#include "android-base/logging.h"
#include "android/binder_interface_utils.h"
#include "bluetooth_hal/config/cs_config_loader.h"
#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_session.h"
#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_util.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_monitor.h"
#include "bluetooth_hal/hci_router.h"
#include "bluetooth_hal/util/android_base_wrapper.h"

namespace bluetooth_hal {
namespace extensions {
namespace cs {

namespace {

using ::aidl::android::hardware::bluetooth::ranging::
    BluetoothChannelSoundingParameters;
using ::aidl::android::hardware::bluetooth::ranging::CsSecurityLevel;
using ::aidl::android::hardware::bluetooth::ranging::
    IBluetoothChannelSoundingSession;
using ::aidl::android::hardware::bluetooth::ranging::
    IBluetoothChannelSoundingSessionCallback;
using ::aidl::android::hardware::bluetooth::ranging::Reason;
using ::aidl::android::hardware::bluetooth::ranging::SessionType;
using ::aidl::android::hardware::bluetooth::ranging::VendorSpecificData;
using ::bluetooth_hal::config::CsConfigLoader;

using ::bluetooth_hal::config::CsConfigLoader;
using ::bluetooth_hal::hci::EventCode;
using ::bluetooth_hal::hci::EventResultCode;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciBleMetaEventMonitor;
using ::bluetooth_hal::hci::HciConstants;
using ::bluetooth_hal::hci::HciMonitor;
using ::bluetooth_hal::hci::HciRouter;
using ::bluetooth_hal::hci::MonitorMode;
using ::bluetooth_hal::hci::MonitorType;
using ::bluetooth_hal::util::AndroidBaseWrapper;

using ::ndk::SharedRefBase;

void SendFakeRasNotification(
    const BluetoothChannelSoundingParameters& parameters,
    int procedure_counter) {
  HalPacket packet = BuildRasNotification(parameters, procedure_counter);
  HciRouter::GetRouter().SendPacketToStack(packet);
}

}  // namespace

BluetoothChannelSoundingHandler::BluetoothChannelSoundingHandler()
    : cs_data_subevent_monitor_(kLeCsSubEventResultCode),
      cs_procedure_enable_subevent_monitor_(kLeCsProcedureEnableCompleteCode) {
  RegisterMonitor(cs_data_subevent_monitor_, MonitorMode::kMonitor);
  RegisterMonitor(cs_procedure_enable_subevent_monitor_, MonitorMode::kMonitor);
}

BluetoothChannelSoundingHandler::~BluetoothChannelSoundingHandler() {
  UnregisterMonitor(cs_data_subevent_monitor_);
  UnregisterMonitor(cs_procedure_enable_subevent_monitor_);
}

bool BluetoothChannelSoundingHandler::GetVendorSpecificData(
    std::optional<std::vector<std::optional<VendorSpecificData>>>*
        return_value) {
  *return_value = std::nullopt;
  return true;
}

bool BluetoothChannelSoundingHandler::GetSupportedSessionTypes(
    std::optional<std::vector<SessionType>>* return_value) {
  *return_value = {SessionType::SOFTWARE_STACK_DATA_PARSING};
  return true;
}

bool BluetoothChannelSoundingHandler::GetMaxSupportedCsSecurityLevel(
    CsSecurityLevel* return_value) {
  *return_value = CsSecurityLevel::ONE;
  return true;
}

bool BluetoothChannelSoundingHandler::OpenSession(
    const BluetoothChannelSoundingParameters& in_params,
    const std::shared_ptr<IBluetoothChannelSoundingSessionCallback>&
        in_callback,
    std::shared_ptr<IBluetoothChannelSoundingSession>* return_value) {
  if (in_params.vendorSpecificData.has_value()) {
    for (auto& data : in_params.vendorSpecificData.value()) {
      LOG(INFO) << "vendorSpecificData uuid:" << ToHex(data->characteristicUuid)
                << ", data:" << ToHex(data->opaqueValue);
    }
  }

  if (IsUuidMatched(in_params.vendorSpecificData) &&
      in_params.vendorSpecificData.value()[0].value().opaqueValue[0] ==
          kDataTypeReply) {
    // Ignore vendor specific reply.
    return true;
  }

  std::shared_ptr<BluetoothChannelSoundingSession> session =
      SharedRefBase::make<BluetoothChannelSoundingSession>(
          in_callback, Reason::LOCAL_STACK_REQUEST);
  session->HandleVendorSpecificData(in_params.vendorSpecificData);
  SessionTracker tracker{.parameters = in_params};

  if (session->ShouldEnableFakeNotification()) {
    LOG(INFO) << __func__ << " Enable fake notification";
    tracker.is_fake_notification_enabled = true;
  }

  session_trackers_.insert_or_assign(in_params.aclHandle, tracker);

  if (session->ShouldEnableMode0ChannelMap()) {
    LOG(INFO) << __func__ << " Enable mode 0 channel map";
    HalPacket command = BuildEnableMode0ChannelMapCommand(
        static_cast<uint16_t>(in_params.aclHandle), kCommandValueEnable);
    SendCommand(command);
  }

  *return_value = session;
  in_callback->onOpened(Reason::LOCAL_STACK_REQUEST);

  return true;
}

void BluetoothChannelSoundingHandler::OnBluetoothEnabled() {
  auto& cs_loader = CsConfigLoader::GetLoader();
  cs_loader.LoadConfig();
  const std::vector<HalPacket>& calibration_commands =
      cs_loader.GetCsCalibrationCommands();

  if (calibration_commands.empty()) {
    LOG(WARNING) << __func__ << ": No calibration commands are found.";
    return;
  }

  for (const auto& command : calibration_commands) {
    SendCommand(command);
  }
};

void BluetoothChannelSoundingHandler::OnBluetoothDisabled() {};

void BluetoothChannelSoundingHandler::OnCommandCallback(
    const HalPacket& packet) {
  // Currently, two command types are supported:
  // 1) Calibration commands (opcode: 0xfd64).
  // 2) Ranging setting commands (opcode: 0xff0b).

  bool status = packet.GetCommandCompleteEventResult() ==
                static_cast<uint8_t>(EventResultCode::kSuccess);

  LOG(status ? INFO : WARNING)
      << __func__ << ": Recv VSE <" << packet.ToString() << "> "
      << (status ? "[Success]" : "[Failed]");

  if (!status ||
      packet.GetCommandOpcodeFromGeneratedEvent() !=
          kHciVscSpecialRangingSettingOpcode ||
      HciConstants::kHciCommandCompleteResultOffset + 1 >= packet.size()) {
    return;
  }

  uint8_t sub_opcode =
      packet[HciConstants::kHciCommandCompleteResultOffset + 1];

  // Store the read local cap value for Stack to read via
  // GetVendorSpecificData.
  if (sub_opcode == kHciVscReadLocalCapabilitySubOpCode) {
    local_capabilities_.clear();
    for (int i = 0; i < kCommandCompleteReadLocalCapabilityValueLength; i++) {
      local_capabilities_.push_back(
          packet[kCommandCompleteReadLocalCapabilityOffset + i]);
    }
  }
};

void BluetoothChannelSoundingHandler::HandleCsSubevent(
    const HalPacket& packet) {
  // [event_type (1 byte)] [event_code (1 byte)] [length (1 byte)]
  // [subevent_code (1 byte)] [connection_handle (2 bytes)].
  uint8_t offset = HciConstants::kHciBleEventSubCodeOffset + 1;
  uint16_t connection_handle =
      packet[offset] + ((packet[offset + 1] << 8u) & 0xff00);

  const auto tracker = GetTracker(connection_handle);
  if (!tracker || !tracker->get().is_fake_notification_enabled) {
    return;
  }

  // Skip config_id, start_acl_conn_event_counter.
  offset += 5;
  uint16_t procedure_counter =
      packet[offset] + ((packet[offset + 1] << 8u) & 0xff00);

  if (tracker->get().cur_procedure_counter == procedure_counter) {
    LOG(DEBUG) << __func__
               << "Skip duplicate fake notification, procedure_counter: "
               << procedure_counter;
    return;
  }

  LOG(DEBUG) << __func__ << "Send fake notification, connection_handle:"
             << connection_handle
             << ", procedure_counter:" << procedure_counter;

  tracker->get().cur_procedure_counter = procedure_counter;
  SendFakeRasNotification(tracker->get().parameters,
                          tracker->get().cur_procedure_counter);
}

void BluetoothChannelSoundingHandler::HandleCsProcedureEnableCompleteEvent(
    const HalPacket& packet) {
  // [event_type (1 byte)] [event_code (1 byte)] [length (1 byte)]
  // [subevent_code (1 byte)] [status (1 byte)][connection_handle (2
  // bytes)].
  uint8_t offset = HciConstants::kHciBleEventSubCodeOffset + 2;
  uint16_t connection_handle =
      packet[offset] + ((packet[offset + 1] << 8u) & 0xff00);

  const auto tracker = GetTracker(connection_handle);
  if (!tracker || !tracker->get().is_fake_notification_enabled) {
    return;
  }
  tracker->get().cur_procedure_counter = kInitialProcedureCounter;
}

void BluetoothChannelSoundingHandler::OnMonitorPacketCallback(
    [[maybe_unused]] MonitorMode mode, const HalPacket& packet) {
  uint8_t subevent_code = packet.GetBleSubEventCode();
  switch (subevent_code) {
    case kLeCsSubEventResultCode:
      HandleCsSubevent(packet);
      break;
    case kLeCsProcedureEnableCompleteCode:
      HandleCsProcedureEnableCompleteEvent(packet);
      break;
    default:
      break;
  }
};

std::optional<
    std::reference_wrapper<BluetoothChannelSoundingHandler::SessionTracker>>
BluetoothChannelSoundingHandler::GetTracker(uint16_t connection_handle) {
  auto it = session_trackers_.find(connection_handle);
  if (it == session_trackers_.end()) {
    return std::nullopt;
  }
  return it->second;
}

}  // namespace cs
}  // namespace extensions
}  // namespace bluetooth_hal
