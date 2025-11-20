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

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "bluetooth_hal/hal_types.h"

namespace bluetooth_hal {
namespace hci {

class HalPacket : public std::vector<uint8_t> {
 public:
  static constexpr size_t kPartialStringSize = 16;
  static constexpr size_t kFullStringSize = 10000;

  /**
   * @brief The default constructors from std::vector.
   *
   */
  HalPacket() = default;
  HalPacket(const std::vector<uint8_t>& other) : std::vector<uint8_t>(other) {}

  /**
   * @brief A constructor of creating a HalPacket with packet type
   * and its payload.
   *
   * @param type The packet type.
   * @param payload The packet payload.
   *
   */
  HalPacket(uint8_t type, const std::vector<uint8_t>& payload) {
    resize(payload.size() + 1);
    at(0) = type;
    std::copy(payload.begin(), payload.end(), begin() + 1);
  }

  /**
   * @brief Support getting the byte at an offset with other types.
   *
   * @param offset Template of the offset, can be enum or other numeric types.
   * @return the byte of the offset in uint8_t.
   *
   */
  template <typename T>
  uint8_t At(T offset) const {
    size_t index = static_cast<size_t>(offset);

    if (index > size()) {
      return 0;
    }

    return at(index);
  }

  /**
   * @brief Support getting two bytes starting at an offset in little endian.
   * Can be used to get packet opcodes or event codes from the packet.
   *
   * @param offset Template of the offset, can be enum or other numeric types.
   * @return the two bytes starting at the offset in little endian uint16_t.
   *
   */
  template <typename T>
  uint16_t AtUint16LittleEndian(T offset) const {
    size_t start_index = static_cast<size_t>(offset);

    if (start_index + 1 >= size()) {
      return 0;
    }

    uint8_t byte1 = at(start_index);
    uint8_t byte2 = at(start_index + 1);

    return static_cast<uint16_t>((byte2 << 8) | byte1);
  }

  /**
   * @brief Support getting eight bytes starting at an offset in little endian.
   *
   * @param offset Template of the offset, can be enum or other numeric types.
   * @return the two bytes starting at the offset in little endian uint64_t.
   *
   */
  template <typename T>
  uint64_t AtUint64LittleEndian(T offset) const {
    size_t start_index = static_cast<size_t>(offset);
    constexpr int kNumOfBytes = sizeof(uint64_t);

    if (start_index + (kNumOfBytes - 1) >= size()) {
      return 0;
    }

    uint64_t result = 0;
    for (int i = 0; i < kNumOfBytes; ++i) {
      uint8_t byte = at(start_index + i);
      result |= (static_cast<uint64_t>(byte) << (i * 8));
    }

    return result;
  }

  /**
   * @brief Print the payload in the HalPacket. Used for debug purposes.
   *
   * @return The string payload in hexdecimal.
   *
   */
  std::string ToFullString() const { return ToString(kFullStringSize); }

  /**
   * @brief Returns a string representation of the first 16 bytes of the packet.
   * If the packet has less than 16 bytes, it returns the entire packet.
   *
   * @return A string containing the hexadecimal representation of the first 16
   * bytes.
   */
  std::string ToString() const { return ToString(kPartialStringSize); }

  /**
   * @brief Get the type of the packet. The type is defined in HciPacketType.
   *
   * @return The type of the packet.
   *
   */
  HciPacketType GetType() const {
    if (empty()) {
      return HciPacketType::kUnknown;
    }
    uint8_t type = front();
    if ((type >= static_cast<uint8_t>(HciPacketType::kCommand) &&
         type <= static_cast<uint8_t>(HciPacketType::kIsoData)) ||
        type == static_cast<uint8_t>(HciPacketType::kThreadData) ||
        type == static_cast<uint8_t>(HciPacketType::kHdlcData)) {
      return static_cast<HciPacketType>(type);
    }

    return HciPacketType::kUnknown;
  }

  /**
   * @brief Get the body of the packet without the first Type byte.
   *
   * @return The body payload in vector<uint8_t>.
   *
   */
  std::vector<uint8_t> GetBody() const {
    if (size() <= 1) {
      return {};
    }
    return std::vector(begin() + 1, end());
  }

  /* APIs for HCI commands */

  /**
   * @brief Get the command opcode of the packet if it is a HCI command.
   *
   * @return The command opcode. 0 if the packet is not a valid HCI command.
   *
   */
  uint16_t GetCommandOpcode() const {
    if (GetType() != HciPacketType::kCommand) {
      return 0;
    }
    return size() > HciConstants::kHciCommandOpcodeOffset + 1
               ? AtUint16LittleEndian(HciConstants::kHciCommandOpcodeOffset)
               : 0;
  }

  /**
   * @brief Check if the packet is a vendor HCI command.
   *
   * @return true of it is a vendor command, otherwise false.
   *
   */
  bool IsVendorCommand() const {
    return (GetCommandOpcode() &
            static_cast<uint16_t>(CommandOpCode::kVendorSpecific)) ==
           static_cast<uint16_t>(CommandOpCode::kVendorSpecific);
  }

  /* APIs for HCI events */

  /**
   * @brief Get event code of the packet, if the packet is a HCI event.
   *
   * @return The event code of the packet. 0 if the packet is not a valid HCI
   * event.
   *
   */
  uint8_t GetEventCode() const {
    if (GetType() != HciPacketType::kEvent ||
        size() <= HciConstants::kHciEventCodeOffset) {
      return 0;
    }
    return at(HciConstants::kHciEventCodeOffset);
  }

  /**
   * @brief Check if the packet is a vendor HCI event.
   *
   * @return true if the packet is a vendor HCI event, otherwise false.
   *
   */
  bool IsVendorEvent() const {
    return GetEventCode() == static_cast<uint8_t>(EventCode::kVendorSpecific);
  }

  /**
   * @brief Check if the packet is a command complete event.
   *
   * @return true if the packet is a command complete event, otherwise false.
   *
   */
  bool IsCommandCompleteEvent() const {
    return (GetEventCode() ==
            static_cast<uint8_t>(EventCode::kCommandComplete)) &&
           size() > HciConstants::kHciCommandCompleteResultOffset;
  }

  /**
   * @brief Check if the packet is a command status event.
   *
   * @return true if the packet is a command status event, otherwise false.
   *
   */
  bool IsCommandStatusEvent() const {
    return GetEventCode() == static_cast<uint8_t>(EventCode::kCommandStatus) &&
           size() > HciConstants::kHciCommandStatusResultOffset;
  }

  /**
   * @brief Get the event result if the packet is a command complete event or a
   * command status event
   *
   * @return The event result in uint8.
   *
   */
  uint8_t GetCommandCompleteEventResult() const {
    uint8_t result = static_cast<uint8_t>(EventResultCode::kFailure);
    if (IsCommandCompleteEvent()) {
      result = at(HciConstants::kHciCommandCompleteResultOffset);
    } else if (IsCommandStatusEvent()) {
      result = at(HciConstants::kHciCommandStatusResultOffset);
    }
    return result;
  }

  /**
   * @brief Check if the packet is a command complete event or command status
   * event.
   *
   * @return true if the packet is a command complete event or command status
   * event, otherwise false.
   *
   */
  bool IsCommandCompleteStatusEvent() const {
    return (IsCommandCompleteEvent() || IsCommandStatusEvent());
  }

  /**
   * @brief Get the command opcode from a command complete event or a command
   * status event.
   *
   * @return The command opcode if the packet is a command complete event or
   * command status event, otherwise return 0.
   *
   */
  uint16_t GetCommandOpcodeFromGeneratedEvent() const {
    if (!IsCommandCompleteStatusEvent()) {
      return 0;
    }

    int offset = IsCommandCompleteEvent()
                     ? HciConstants::kHciCommandCompleteCommandOpcodeOffset
                     : HciConstants::kHciCommandStatusCommandOpcodeOffset;

    return AtUint16LittleEndian(offset);
  }

  /* APIs for BLE events */

  /**
   * @brief Check if the packet is a BLE meta event.
   *
   * @return true if the packet is a BLE meta event, otherwise false.
   *
   */
  bool IsBleMetaEvent() const {
    return GetEventCode() == static_cast<uint8_t>(EventCode::kBleMeta);
  }

  /**
   * @brief Get the BLE sub-event code if the packet is a BLE meta event.
   *
   * @return The BLE sub-event code of the packet. 0 if the packet is not a
   * valid BLE meta event.
   *
   */
  uint8_t GetBleSubEventCode() const {
    if (!IsBleMetaEvent()) {
      return 0;
    }
    return size() > HciConstants::kHciBleEventSubCodeOffset
               ? at(HciConstants::kHciBleEventSubCodeOffset)
               : 0;
  }

 private:
  std::string ToString(size_t string_size) const {
    std::stringstream ss;
    size_t output_size = std::min(size(), string_size);
    ss << "(" << size() << ")[";
    for (size_t i = 0; i < output_size; ++i) {
      ss << std::hex << std::setw(2) << std::setfill('0')
         << static_cast<int>(at(i));
      if (i < size() - 1) {
        ss << " ";
      }
    }
    if (output_size < size()) {
      ss << "... ";
    }
    ss << "]";
    return ss.str();
  }
};

/**
 * @brief Type alias for packet routing callbacks.
 *
 * This type alias defines the signature for all callback functions used
 * in packet routing.  Any function that needs to be notified about
 * new packets should be compatible with this type.
 *
 * The callback function should take a single argument: a constant
 * reference to a `HalPacket` object, which represents the received packet.
 * The callback function does not return any value.
 *
 */
using HalPacketCallback = std::function<void(const HalPacket& packet)>;

}  // namespace hci
}  // namespace bluetooth_hal
