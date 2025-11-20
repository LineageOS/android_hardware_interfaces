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

#define LOG_TAG "bthal.extensions.ccc"

#include "bluetooth_hal/extensions/ccc/bluetooth_ccc.h"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "aidl/hardware/google/bluetooth/ccc/BnBluetoothCcc.h"
#include "aidl/hardware/google/bluetooth/ccc/IBluetoothCccCallback.h"
#include "android-base/logging.h"
#include "android/binder_auto_utils.h"
#include "android/binder_status.h"
#include "bluetooth_hal/bluetooth_address.h"
#include "bluetooth_hal/extensions/ccc/bluetooth_ccc_handler.h"
#include "bluetooth_hal/extensions/ccc/bluetooth_ccc_util.h"

namespace bluetooth_hal {
namespace extensions {
namespace ccc {
namespace {

using ::aidl::hardware::google::bluetooth::ccc::Direction;
using ::aidl::hardware::google::bluetooth::ccc::IBluetoothCccCallback;
using ::aidl::hardware::google::bluetooth::ccc::LmpEventId;
using ::aidl::hardware::google::bluetooth::ccc::Timestamp;
using ::bluetooth_hal::hci::BluetoothAddress;
using ::ndk::ScopedAStatus;

using ScopedDeathRecipient =
    std::unique_ptr<AIBinder_DeathRecipient,
                    void (*)(AIBinder_DeathRecipient*)>;

ScopedDeathRecipient MakeScopedDeathRecipient(
    AIBinder_DeathRecipient* death_recipient) {
  return ScopedDeathRecipient(death_recipient, &AIBinder_DeathRecipient_delete);
}

class BluetoothCccDeathRecipient {
 public:
  BluetoothCccDeathRecipient(const BluetoothAddress& address)
      : is_dead_(false),
        ccc_callback_(nullptr),
        client_death_recipient_(MakeScopedDeathRecipient(nullptr)),
        address_(address) {}

  void LinkToDeath(const std::shared_ptr<IBluetoothCccCallback>& cb) {
    ccc_callback_ = cb;

    auto on_link_died = [](void* cookie) {
      auto* death_recipient = static_cast<BluetoothCccDeathRecipient*>(cookie);
      death_recipient->ServiceDied();
    };
    client_death_recipient_ =
        MakeScopedDeathRecipient(AIBinder_DeathRecipient_new(on_link_died));

    binder_status_t link_to_death_return_status =
        AIBinder_linkToDeath(ccc_callback_->asBinder().get(),
                             client_death_recipient_.get(), this /* cookie */);
    if (link_to_death_return_status != STATUS_OK) {
      LOG(FATAL) << "Unable to link to death recipient";
    }
  }

  void UnlinkToDeath() {
    if (!is_dead_) {
      binder_status_t unlink_to_death_return_status = AIBinder_unlinkToDeath(
          ccc_callback_->asBinder().get(), client_death_recipient_.get(), this);
      if (unlink_to_death_return_status != STATUS_OK) {
        LOG(FATAL) << "Unable to unlink to death recipient";
      }
    }
    client_death_recipient_.reset();
  }

  void ServiceDied() {
    LOG(WARNING) << __func__ << ": BluetoothCccDeathRecipient::serviceDied";
    is_dead_ = true;
    BluetoothCccHandler::GetHandler().UnregisterLmpEvents(
        BluetoothAddress(address_));
  }

 private:
  bool is_dead_;
  std::shared_ptr<IBluetoothCccCallback> ccc_callback_;
  ScopedDeathRecipient client_death_recipient_;
  BluetoothAddress address_;
};

class BluetoothCccHandlerCallbackImpl : public BluetoothCccHandlerCallback {
 public:
  explicit BluetoothCccHandlerCallbackImpl(
      const std::shared_ptr<IBluetoothCccCallback>& bluetooth_ccc_callback,
      const BluetoothAddress& address,
      const std::vector<CccLmpEventId>& lmp_event_ids,
      const std::shared_ptr<BluetoothCccDeathRecipient> death_recipient)
      : BluetoothCccHandlerCallback(address, lmp_event_ids),
        bluetooth_ccc_callback_(bluetooth_ccc_callback),
        death_recipient_(death_recipient) {
    death_recipient_->LinkToDeath(bluetooth_ccc_callback_);
  }

  ~BluetoothCccHandlerCallbackImpl() { death_recipient_->UnlinkToDeath(); }

  void OnEventGenerated(const CccTimestamp& timestamp,
                        const BluetoothAddress& address, CccDirection direction,
                        CccLmpEventId lmp_event_id,
                        uint8_t event_counter) override {
    if (bluetooth_ccc_callback_ == nullptr) {
      return;
    }
    bluetooth_ccc_callback_->onEventGenerated(
        Timestamp(timestamp.system_time, timestamp.bluetooth_time), address,
        static_cast<Direction>(direction),
        static_cast<LmpEventId>(lmp_event_id), event_counter);
  }

  void OnRegistered(bool status) override {
    if (bluetooth_ccc_callback_ == nullptr) {
      return;
    }
    bluetooth_ccc_callback_->onRegistered(status);
  }

 private:
  const std::shared_ptr<IBluetoothCccCallback>& bluetooth_ccc_callback_;
  const std::shared_ptr<BluetoothCccDeathRecipient> death_recipient_;
};

std::vector<CccLmpEventId> LmpEventCast(
    const std::vector<LmpEventId>& event_ids) {
  std::vector<CccLmpEventId> ccc_event_ids;
  for (const auto& event_id : event_ids) {
    ccc_event_ids.push_back(static_cast<CccLmpEventId>(event_id));
  }
  return ccc_event_ids;
}

}  // namespace

ScopedAStatus BluetoothCcc::registerForLmpEvents(
    const std::shared_ptr<IBluetoothCccCallback>& callback,
    const std::array<uint8_t, 6>& address,
    const std::vector<LmpEventId>& lmpEventIds) {
  const auto lmp_event_ids = LmpEventCast(lmpEventIds);
  const auto bluetooth_address = BluetoothAddress(address);
  const auto death_recipient =
      std::make_shared<BluetoothCccDeathRecipient>(bluetooth_address);
  bool status = BluetoothCccHandler::GetHandler().RegisterForLmpEvents(
      std::make_unique<BluetoothCccHandlerCallbackImpl>(
          callback, bluetooth_address, lmp_event_ids, death_recipient));
  return status ? ScopedAStatus::ok()
                : ScopedAStatus::fromExceptionCode(EX_NULL_POINTER);
}

ScopedAStatus BluetoothCcc::unregisterLmpEvents(
    const std::array<uint8_t, 6>& address) {
  bool status = BluetoothCccHandler::GetHandler().UnregisterLmpEvents(
      BluetoothAddress(address));
  return status ? ScopedAStatus::ok()
                : ScopedAStatus::fromExceptionCode(EX_NULL_POINTER);
}

}  // namespace ccc
}  // namespace extensions
}  // namespace bluetooth_hal
