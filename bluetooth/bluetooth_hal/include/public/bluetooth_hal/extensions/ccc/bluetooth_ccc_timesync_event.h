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

#pragma once

#include <cstdint>

#include "bluetooth_hal/bluetooth_address.h"
#include "bluetooth_hal/extensions/ccc/bluetooth_ccc_util.h"
#include "bluetooth_hal/hal_packet.h"

namespace bluetooth_hal {
namespace extensions {
namespace ccc {

class BluetoothCccTimesyncEvent : public ::bluetooth_hal::hci::HalPacket {
 public:
  /**
   * @brief Constructs a BluetoothCccTimesyncEvent object from a raw packet.
   *
   * Parses the provided `HalPacket` data according to the CCC Timesync Event
   * format. The `IsValid()` method can be used post-construction to check if
   * parsing was successful.
   *
   * @param packet The raw `::bluetooth_hal::hci::HalPacket` containing the
   * event data.
   */
  explicit BluetoothCccTimesyncEvent(
      const ::bluetooth_hal::hci::HalPacket& packet);

  /**
   * @brief Check if the packet is a valid CCC time sync event.
   *
   * @return true if the packet is a CCC time sync event, otherwise false.
   *
   */
  bool IsValid();

  /**
   * @brief Retrieves the Bluetooth address from the event packet.
   *
   * @return The 6-byte Bluetooth address as
   * `::bluetooth_hal::hci::BluetoothAddress`.
   */
  ::bluetooth_hal::hci::BluetoothAddress GetAddress() const;

  /**
   * @brief Retrieves the address type from the event packet.
   *
   * @return The 1-byte address type.
   */
  uint8_t GetAddressType() const;

  /**
   * @brief Retrieves the direction of the event (Tx, Rx or Undefined).
   *
   * @return The direction as a `CccDirection` enum value.
   */
  CccDirection GetDirection() const;

  /**
   * @brief Retrieves the timestamp from the event packet.
   *
   * @return The 8-byte timestamp as a `uint64_t`.
   */
  uint64_t GetTimestamp() const;

  /**
   * @brief Retrieves the LMP (Link Manager Protocol) event ID.
   *
   * This ID is mapped from the raw byte in the packet to a `CccLmpEventId`
   * enum. Specific raw byte values map to defined enum members, otherwise it's
   * `kUndefined`.
   *
   * @return The LMP event ID as a `CccLmpEventId` enum value.
   */
  CccLmpEventId GetEventId() const;

  /**
   * @brief Retrieves the toggle count from the event packet.
   *
   * @return The 1-byte toggle count as a `uint8_t`.
   */
  uint8_t GetToggleCount() const;

  /**
   * @brief Retrieves the timesync offset from the event packet.
   *
   * @return The 2-byte timesync offset as a `uint16_t`.
   */
  uint16_t GetTimesyncOffset() const;

  /**
   * @brief Retrieves the event count from the event packet.
   *
   * @return The 2-byte event count as a `uint16_t`.
   */
  uint16_t GetEventCount() const;

 private:
  bool is_valid_;
  ::bluetooth_hal::hci::BluetoothAddress address_;
  uint8_t address_type_;
  uint8_t direction_;
  uint64_t timestamp_;
  uint8_t event_id_;
  uint8_t toggle_count_;
  uint16_t timesync_offset_;
  uint16_t event_count_;
};

}  // namespace ccc
}  // namespace extensions
}  // namespace bluetooth_hal
