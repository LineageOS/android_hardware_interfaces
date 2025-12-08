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

#include <algorithm>
#include <cstdint>
#include <vector>

#include "bluetooth_hal/bluetooth_address.h"
#include "bluetooth_hal/extensions/ccc/bluetooth_ccc_util.h"

namespace bluetooth_hal {
namespace extensions {
namespace ccc {

class BluetoothCccHandlerCallback {
 public:
  BluetoothCccHandlerCallback(
      const ::bluetooth_hal::hci::BluetoothAddress& address,
      const std::vector<CccLmpEventId>& lmp_event_ids)
      : address_(address),
        address_type_(AddressType::kRandom),
        lmp_event_ids_(lmp_event_ids) {};
  BluetoothCccHandlerCallback(
      const ::bluetooth_hal::hci::BluetoothAddress& address,
      const AddressType address_type,
      const std::vector<CccLmpEventId>& lmp_event_ids)
      : address_(address),
        address_type_(address_type),
        lmp_event_ids_(lmp_event_ids) {};
  virtual ~BluetoothCccHandlerCallback() = default;
  virtual void OnEventGenerated(
      const CccTimestamp& timestamp,
      const ::bluetooth_hal::hci::BluetoothAddress& address,
      CccDirection direction, CccLmpEventId lmp_event_id,
      uint8_t event_counter) = 0;

  virtual void OnRegistered(bool status) = 0;

  bool ContainsEventId(CccLmpEventId lmp_event_id) const {
    return std::find(lmp_event_ids_.begin(), lmp_event_ids_.end(),
                     lmp_event_id) != lmp_event_ids_.end();
  }

  bool IsAddressEqual(const ::bluetooth_hal::hci::BluetoothAddress& address,
                      const AddressType address_type) const {
    return ((address_ == address) && (address_type_ == address_type));
  }

  const ::bluetooth_hal::hci::BluetoothAddress& GetAddress() const {
    return address_;
  }

  AddressType GetAddressType() const { return address_type_; }

  const std::vector<CccLmpEventId>& GetLmpEventIds() const {
    return lmp_event_ids_;
  }

 private:
  const ::bluetooth_hal::hci::BluetoothAddress address_;
  const AddressType address_type_;
  const std::vector<CccLmpEventId> lmp_event_ids_;
};

}  // namespace ccc
}  // namespace extensions
}  // namespace bluetooth_hal
