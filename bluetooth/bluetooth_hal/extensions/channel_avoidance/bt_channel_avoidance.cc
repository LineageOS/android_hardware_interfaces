/*
 * Copyright 2023 The Android Open Source Project
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

#define LOG_TAG "bthal.aidl.channel_avoidance"

#include "bluetooth_hal/extensions/channel_avoidance/bt_channel_avoidance.h"

#include "android-base/logging.h"
#include "bluetooth_hal/debug/vnd_snoop_logger.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"

namespace vendor {
namespace google {
namespace bluetooth_ext {
namespace bt_channel_avoidance {
namespace aidl {
namespace implementation {

using ::bluetooth_hal::debug::VndSnoopLogger;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciPacketType;

using VndLogDirection = ::bluetooth_hal::debug::VndSnoopLogger::Direction;

// HCI_Set_AFH_Host_Channel_Classification: OGF 0x03 | OCF 0x003F
constexpr uint16_t kHciChannelAvoidanceOpcode = 0x0c3f;
constexpr uint8_t kHciCommandOpcodeLength = 2;
constexpr uint8_t kHciChannelAvoidanceMapSize = 10;
constexpr size_t kHciChannelAvoidanceCmdLength =
    kHciCommandOpcodeLength + 1 + kHciChannelAvoidanceMapSize;

constexpr uint8_t kCommandCompleteStatusOffset =
    HciEventWatcher::kCommandCompleteOpcodeOffset + kHciCommandOpcodeLength;
constexpr uint8_t kCommandCompleteStatusSucceed = 0x00;

HciFlowControl* BTChannelAvoidance::hci_handle_ = nullptr;
BTChannelAvoidance BTChannelAvoidance::instance_;

BTChannelAvoidance::BTChannelAvoidance()
    : HciEventWatcher(LOG_TAG, HciEventWatcher::kCommandCompleteEventCode,
                      kHciChannelAvoidanceOpcode, false, true) {}

ndk::ScopedAStatus BTChannelAvoidance::setBluetoothChannelStatus(
    const std::array<uint8_t, 10>& channel_map) {
  if (hci_handle_ == nullptr) {
    LOG(WARNING) << __func__ << ": Unable to set channel map <"
                 << channel_map.data() << ">";
    return ndk::ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
  }
  LOG(INFO) << __func__ << ": Channel Map <" << channel_map.data() << ">";
  std::vector<uint8_t> packet_body;
  packet_body.resize(kHciChannelAvoidanceCmdLength);
  packet_body[0] = kHciChannelAvoidanceOpcode & 0xff;
  packet_body[1] = (kHciChannelAvoidanceOpcode >> 8u) & 0xff;
  packet_body[2] = kHciChannelAvoidanceMapSize;
  for (int i = 0; i < kHciChannelAvoidanceMapSize; ++i) {
    packet_body[3 + i] = channel_map[i];
  }
  HalPacket packet(static_cast<uint8_t>(HciPacketType::kCommand), packet_body);
  ++(instance_.event_waiting_);
  if (hci_handle_ != nullptr) {
    hci_handle_->Send(static_cast<uint8_t>(packet.GetType()),
                      packet_body.data(), packet_body.size());
  } else {
    LOG(WARNING) << __func__ << ": Unable to send channel map vsc";
    return ndk::ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
  }
  return ndk::ScopedAStatus::ok();
}

bool BTChannelAvoidance::OnEventReceive(const HalPacket& event) {
  bool status =
      event[kCommandCompleteStatusOffset] == kCommandCompleteStatusSucceed;
  unsigned int waiting_count = event_waiting_.load(std::memory_order_relaxed);
  if (status) {
    LOG(INFO) << __func__ << ": (" << waiting_count << ") Recv Success VSE <"
              << event.ToString() << ">";
  } else {
    LOG(WARNING) << __func__ << ": (" << waiting_count << ") Recv Failure VSE <"
                 << event.ToString() << ">";
  }
  if (!waiting_count) return false;
  --event_waiting_;
  return true;
}

bool BTChannelAvoidance::OnEventPost(const HalPacket& event) {
  (void)event;
  return true;
}

void BTChannelAvoidance::OnBluetoothEnabled(HciFlowControl* handle) {
  LOG(DEBUG) << __func__;
  hci_handle_ = handle;
  if (handle != nullptr) {
    hci_handle_->RegisterEventWatcher(&instance_);
  }
}

void BTChannelAvoidance::OnBluetoothDisabled() {
  LOG(DEBUG) << __func__;
  if (hci_handle_ != nullptr) {
    hci_handle_->UnregisterEventWatcher(&instance_);
    hci_handle_ = nullptr;
  }
}

}  // namespace implementation
}  // namespace aidl
}  // namespace bt_channel_avoidance
}  // namespace bluetooth_ext
}  // namespace google
}  // namespace vendor
