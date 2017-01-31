//
// Copyright 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#define LOG_TAG "android.hardware.bluetooth@1.0-impl"
#include <utils/Log.h>

#include "bluetooth_hci.h"
#include "vendor_interface.h"

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace implementation {

static const uint8_t HCI_DATA_TYPE_COMMAND = 1;
static const uint8_t HCI_DATA_TYPE_ACL = 2;
static const uint8_t HCI_DATA_TYPE_SCO = 3;

Return<void> BluetoothHci::initialize(
    const ::android::sp<IBluetoothHciCallbacks>& cb) {
  ALOGW("BluetoothHci::initialize()");
  event_cb_ = cb;

  bool rc = VendorInterface::Initialize(
      [this](bool status) {
        event_cb_->initializationComplete(
            status ? Status::SUCCESS : Status::INITIALIZATION_ERROR);
      },
      [this](HciPacketType type, const hidl_vec<uint8_t>& packet) {
        switch (type) {
          case HCI_PACKET_TYPE_EVENT:
            event_cb_->hciEventReceived(packet);
            break;
          case HCI_PACKET_TYPE_ACL_DATA:
            event_cb_->aclDataReceived(packet);
            break;
          case HCI_PACKET_TYPE_SCO_DATA:
            event_cb_->scoDataReceived(packet);
            break;
          default:
            ALOGE("%s Unexpected event type %d", __func__, type);
            break;
        }
      });
  if (!rc) event_cb_->initializationComplete(Status::INITIALIZATION_ERROR);
  return Void();
}

Return<void> BluetoothHci::close() {
  ALOGW("BluetoothHci::close()");
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

void BluetoothHci::sendDataToController(const uint8_t type,
                                        const hidl_vec<uint8_t>& data) {
  VendorInterface::get()->Send(type, data.data(), data.size());
}

IBluetoothHci* HIDL_FETCH_IBluetoothHci(const char* /* name */) {
  return new BluetoothHci();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
