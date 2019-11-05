/*
 * Copyright 2016 The Android Open Source Project
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

#define LOG_TAG "android.hardware.bluetooth@1.1-impl"
#include "bluetooth_hci.h"

#include <log/log.h>

#include "vendor_interface.h"

using android::hardware::bluetooth::V1_0::implementation::VendorInterface;

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_1 {
namespace implementation {

static const uint8_t HCI_DATA_TYPE_COMMAND = 1;
static const uint8_t HCI_DATA_TYPE_ACL = 2;
static const uint8_t HCI_DATA_TYPE_SCO = 3;
static const uint8_t HCI_DATA_TYPE_ISO = 5;

class BluetoothDeathRecipient : public hidl_death_recipient {
 public:
  BluetoothDeathRecipient(const sp<IBluetoothHci> hci) : mHci(hci) {}

  virtual void serviceDied(
      uint64_t /*cookie*/,
      const wp<::android::hidl::base::V1_0::IBase>& /*who*/) {
    ALOGE("BluetoothDeathRecipient::serviceDied - Bluetooth service died");
    has_died_ = true;
    mHci->close();
  }
  sp<IBluetoothHci> mHci;
  bool getHasDied() const { return has_died_; }
  void setHasDied(bool has_died) { has_died_ = has_died; }

 private:
  bool has_died_;
};

BluetoothHci::BluetoothHci()
    : death_recipient_(new BluetoothDeathRecipient(this)) {}

Return<void> BluetoothHci::initialize_1_1(
    const ::android::sp<V1_1::IBluetoothHciCallbacks>& cb) {
  ALOGI("BluetoothHci::initialize_1_1()");
  if (cb == nullptr) {
    ALOGE("cb == nullptr! -> Unable to call initializationComplete(ERR)");
    return Void();
  }

  death_recipient_->setHasDied(false);
  cb->linkToDeath(death_recipient_, 0);

  bool rc = VendorInterface::Initialize(
      [cb](bool status) {
        auto hidl_status = cb->initializationComplete(
            status ? V1_0::Status::SUCCESS
                   : V1_0::Status::INITIALIZATION_ERROR);
        if (!hidl_status.isOk()) {
          ALOGE("VendorInterface -> Unable to call initializationComplete()");
        }
      },
      [cb](const hidl_vec<uint8_t>& packet) {
        auto hidl_status = cb->hciEventReceived(packet);
        if (!hidl_status.isOk()) {
          ALOGE("VendorInterface -> Unable to call hciEventReceived()");
        }
      },
      [cb](const hidl_vec<uint8_t>& packet) {
        auto hidl_status = cb->aclDataReceived(packet);
        if (!hidl_status.isOk()) {
          ALOGE("VendorInterface -> Unable to call aclDataReceived()");
        }
      },
      [cb](const hidl_vec<uint8_t>& packet) {
        auto hidl_status = cb->scoDataReceived(packet);
        if (!hidl_status.isOk()) {
          ALOGE("VendorInterface -> Unable to call scoDataReceived()");
        }
      },
      [cb](const hidl_vec<uint8_t>& packet) {
        auto hidl_status = cb->isoDataReceived(packet);
        if (!hidl_status.isOk()) {
          ALOGE("VendorInterface -> Unable to call isoDataReceived()");
        }
      });
  if (!rc) {
    auto hidl_status =
        cb->initializationComplete(V1_0::Status::INITIALIZATION_ERROR);
    if (!hidl_status.isOk()) {
      ALOGE("VendorInterface -> Unable to call initializationComplete(ERR)");
    }
  }

  unlink_cb_ = [cb](sp<BluetoothDeathRecipient>& death_recipient) {
    if (death_recipient->getHasDied())
      ALOGI("Skipping unlink call, service died.");
    else
      cb->unlinkToDeath(death_recipient);
  };

  return Void();
}

class OldCbWrapper : public V1_1::IBluetoothHciCallbacks {
 public:
  const ::android::sp<V1_0::IBluetoothHciCallbacks> old_cb_;
  OldCbWrapper(const ::android::sp<V1_0::IBluetoothHciCallbacks>& old_cb)
      : old_cb_(old_cb) {}

  virtual ~OldCbWrapper() = default;

  Return<void> initializationComplete(V1_0::Status status) override {
    return old_cb_->initializationComplete(status);
  };

  Return<void> hciEventReceived(
      const ::android::hardware::hidl_vec<uint8_t>& event) override {
    return old_cb_->hciEventReceived(event);
  };

  Return<void> aclDataReceived(
      const ::android::hardware::hidl_vec<uint8_t>& data) override {
    return old_cb_->aclDataReceived(data);
  };

  Return<void> scoDataReceived(
      const ::android::hardware::hidl_vec<uint8_t>& data) override {
    return old_cb_->scoDataReceived(data);
  };

  Return<void> isoDataReceived(
      const ::android::hardware::hidl_vec<uint8_t>&) override {
    ALOGE("Please use HAL V1_1 for ISO.");
    return Void();
  };
};

Return<void> BluetoothHci::initialize(
    const ::android::sp<V1_0::IBluetoothHciCallbacks>& cb) {
  ALOGE("Using initialize from HAL V1_0 instead of initialize_1_1.");
  return initialize_1_1(new OldCbWrapper(cb));
}

Return<void> BluetoothHci::close() {
  ALOGI("BluetoothHci::close()");
  unlink_cb_(death_recipient_);
  VendorInterface::Shutdown();
  return Void();
}

Return<void> BluetoothHci::sendHciCommand(const hidl_vec<uint8_t>& command) {
  sendDataToController(HCI_DATA_TYPE_COMMAND, command);
  return Void();
}

Return<void> BluetoothHci::sendAclData(const hidl_vec<uint8_t>& data) {
  sendDataToController(HCI_DATA_TYPE_ACL, data);
  return Void();
}

Return<void> BluetoothHci::sendScoData(const hidl_vec<uint8_t>& data) {
  sendDataToController(HCI_DATA_TYPE_SCO, data);
  return Void();
}

Return<void> BluetoothHci::sendIsoData(const hidl_vec<uint8_t>& data) {
  sendDataToController(HCI_DATA_TYPE_ISO, data);
  return Void();
}

void BluetoothHci::sendDataToController(const uint8_t type,
                                        const hidl_vec<uint8_t>& data) {
  VendorInterface::get()->Send(type, data.data(), data.size());
}

}  // namespace implementation
}  // namespace V1_1
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
