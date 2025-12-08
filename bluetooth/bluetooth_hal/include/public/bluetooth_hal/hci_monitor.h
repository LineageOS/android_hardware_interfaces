/*
 * Copyright 2024 The Android Open Source Project
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

#pragma once

#include <cstdint>
#include <map>

#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"

namespace bluetooth_hal {
namespace hci {

// Enum remains the same
enum class MonitorType : int {
  kNone,
  kCommand,
  kEvent,
  kThread,
};

class HciMonitor {
 public:
  HciMonitor(MonitorType type, uint16_t primary_code,
             PacketDestination direction)
      : type_(type), primary_code_(primary_code), direction_(direction) {}

  void MonitorOffset(int offset, uint8_t data) {
    monitor_offset_map_[offset] = data;
  }

  bool operator==(const HciMonitor& other) const {
    return type_ == other.GetType() &&
           primary_code_ == other.GetPrimaryCode() &&
           direction_ == other.GetDestination() &&
           monitor_offset_map_ == other.monitor_offset_map_;
  }

  bool operator==(const ::bluetooth_hal::hci::HalPacket& packet) const {
    uint16_t packet_primary_code = 0;
    bool type_match = false;

    if (packet.GetDestination() != PacketDestination::kNone &&
        packet.GetDestination() != GetDestination()) {
      return false;
    }

    if (packet.GetType() == ::bluetooth_hal::hci::HciPacketType::kCommand &&
        type_ == MonitorType::kCommand) {
      type_match = true;
      packet_primary_code = packet.GetCommandOpcode();
    } else if (packet.GetType() ==
                   ::bluetooth_hal::hci::HciPacketType::kEvent &&
               type_ == MonitorType::kEvent) {
      type_match = true;
      packet_primary_code = packet.GetEventCode();
    } else if (packet.GetType() ==
                   ::bluetooth_hal::hci::HciPacketType::kThreadData &&
               type_ == MonitorType::kThread) {
      type_match = true;
      // ThreadData packets don't have a primary code like opcode or event code.
      // We only match based on type and potential offsets.
      packet_primary_code = primary_code_;
    }

    if (!type_match) {
      return false;
    }

    if (primary_code_ != packet_primary_code) {
      return false;
    }

    for (const auto& pair : monitor_offset_map_) {
      int offset = pair.first;
      uint8_t expected_data = pair.second;
      // Check bounds and data value
      if (offset < 0 || static_cast<size_t>(offset) >= packet.size() ||
          packet[offset] != expected_data) {
        return false;
      }
    }

    return true;
  }

  bool operator<(const HciMonitor& other) const {
    if (type_ != other.type_) {
      return type_ < other.type_;
    }
    if (primary_code_ != other.primary_code_) {
      return primary_code_ < other.primary_code_;
    }
    return monitor_offset_map_ < other.monitor_offset_map_;
  }

 protected:
  MonitorType GetType() const { return type_; }
  uint16_t GetPrimaryCode() const { return primary_code_; }
  PacketDestination GetDestination() const { return direction_; };

  const std::map<int, uint8_t>& GetMonitorOffsets() const {
    return monitor_offset_map_;
  }

 private:
  MonitorType type_;
  uint16_t primary_code_;
  PacketDestination direction_;
  std::map<int, uint8_t> monitor_offset_map_;
};

class HciEventMonitor : public HciMonitor {
 public:
  explicit HciEventMonitor(uint8_t event_code)
      : HciMonitor(MonitorType::kEvent, static_cast<uint16_t>(event_code),
                   PacketDestination::kHost) {}

  HciEventMonitor(uint8_t event_code, uint8_t sub_event_code,
                  int sub_event_offset)
      : HciMonitor(MonitorType::kEvent, static_cast<uint16_t>(event_code),
                   PacketDestination::kHost) {
    MonitorOffset(sub_event_offset, sub_event_code);
  }
};

class HciBleMetaEventMonitor : public HciEventMonitor {
 public:
  explicit HciBleMetaEventMonitor(uint8_t ble_event_code)
      : HciEventMonitor(static_cast<uint8_t>(EventCode::kBleMeta),
                        ble_event_code,
                        HciConstants::kHciBleEventSubCodeOffset) {}
};

class HciBqrEventMonitor : public HciEventMonitor {
 public:
  HciBqrEventMonitor()
      : HciEventMonitor(static_cast<uint8_t>(EventCode::kVendorSpecific),
                        static_cast<uint8_t>(GoogleEventSubCode::kBqrEvent),
                        HciConstants::kHciBqrEventSubCodeOffset) {}
  explicit HciBqrEventMonitor(uint8_t report_id)
      : HciEventMonitor(static_cast<uint8_t>(EventCode::kVendorSpecific),
                        static_cast<uint8_t>(GoogleEventSubCode::kBqrEvent),
                        HciConstants::kHciBqrEventSubCodeOffset) {
    MonitorOffset(HciConstants::kHciBqrReportIdOffset, report_id);
  }
};

class HciCommandCompleteEventMonitor : public HciEventMonitor {
 public:
  HciCommandCompleteEventMonitor(uint16_t command_opcode)
      : HciEventMonitor(static_cast<uint8_t>(EventCode::kCommandComplete)) {
    uint8_t byte1 = static_cast<uint8_t>(command_opcode);
    uint8_t byte2 = static_cast<uint8_t>(command_opcode >> 8);
    MonitorOffset(HciConstants::kHciCommandCompleteCommandOpcodeOffset, byte1);
    MonitorOffset(HciConstants::kHciCommandCompleteCommandOpcodeOffset + 1,
                  byte2);
  }
};

class HciCommandStatusEventMonitor : public HciEventMonitor {
 public:
  HciCommandStatusEventMonitor(uint16_t command_opcode)
      : HciEventMonitor(static_cast<uint8_t>(EventCode::kCommandStatus)) {
    uint8_t byte1 = static_cast<uint8_t>(command_opcode);
    uint8_t byte2 = static_cast<uint8_t>(command_opcode >> 8);
    MonitorOffset(HciConstants::kHciCommandStatusCommandOpcodeOffset, byte1);
    MonitorOffset(HciConstants::kHciCommandStatusCommandOpcodeOffset + 1,
                  byte2);
  }
};

class HciCommandMonitor : public HciMonitor {
 public:
  explicit HciCommandMonitor(uint16_t opcode)
      : HciMonitor(MonitorType::kCommand, opcode,
                   PacketDestination::kController) {}

  HciCommandMonitor(uint16_t opcode, uint8_t sub_opcode, int sub_opcode_offset)
      : HciMonitor(MonitorType::kCommand, opcode,
                   PacketDestination::kController) {
    MonitorOffset(sub_opcode_offset, sub_opcode);
  }
};

class HciThreadMonitor : public HciMonitor {
 public:
  HciThreadMonitor()
      : HciMonitor(MonitorType::kThread, 0, PacketDestination::kHost) {}

  HciThreadMonitor(int offset, uint8_t data) : HciThreadMonitor() {
    MonitorOffset(offset, data);
  }
};

}  // namespace hci
}  // namespace bluetooth_hal
