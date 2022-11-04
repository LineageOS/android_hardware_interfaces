/*
 * Copyright 2022 The Android Open Source Project
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

#define LOG_TAG "android.hardware.bluetooth.service.default"

#include "BluetoothHci.h"

#include <cutils/properties.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <string.h>
#include <sys/uio.h>
#include <termios.h>

#include "log/log.h"

namespace {
int SetTerminalRaw(int fd) {
  termios terminal_settings;
  int rval = tcgetattr(fd, &terminal_settings);
  if (rval < 0) {
    return rval;
  }
  cfmakeraw(&terminal_settings);
  rval = tcsetattr(fd, TCSANOW, &terminal_settings);
  return rval;
}
}  // namespace

using namespace ::android::hardware::bluetooth::hci;
using namespace ::android::hardware::bluetooth::async;

namespace aidl::android::hardware::bluetooth::impl {

void OnDeath(void* cookie);

class BluetoothDeathRecipient {
 public:
  BluetoothDeathRecipient(BluetoothHci* hci) : mHci(hci) {}

  void LinkToDeath(const std::shared_ptr<IBluetoothHciCallbacks>& cb) {
    mCb = cb;
    clientDeathRecipient_ = AIBinder_DeathRecipient_new(OnDeath);
    auto linkToDeathReturnStatus = AIBinder_linkToDeath(
        mCb->asBinder().get(), clientDeathRecipient_, this /* cookie */);
    LOG_ALWAYS_FATAL_IF(linkToDeathReturnStatus != STATUS_OK,
                        "Unable to link to death recipient");
  }

  void UnlinkToDeath(const std::shared_ptr<IBluetoothHciCallbacks>& cb) {
    LOG_ALWAYS_FATAL_IF(cb != mCb, "Unable to unlink mismatched pointers");
  }

  void serviceDied() {
    if (mCb != nullptr && !AIBinder_isAlive(mCb->asBinder().get())) {
      ALOGE("Bluetooth remote service has died");
    } else {
      ALOGE("BluetoothDeathRecipient::serviceDied called but service not dead");
      return;
    }
    has_died_ = true;
    mHci->close();
  }
  BluetoothHci* mHci;
  std::shared_ptr<IBluetoothHciCallbacks> mCb;
  AIBinder_DeathRecipient* clientDeathRecipient_;
  bool getHasDied() const { return has_died_; }

 private:
  bool has_died_{false};
};

void OnDeath(void* cookie) {
  auto* death_recipient = static_cast<BluetoothDeathRecipient*>(cookie);
  death_recipient->serviceDied();
}

BluetoothHci::BluetoothHci(const std::string& dev_path) {
  char property_bytes[PROPERTY_VALUE_MAX];
  property_get("vendor.ser.bt-uart", property_bytes, dev_path.c_str());
  mDevPath = std::string(property_bytes);
  mDeathRecipient = std::make_shared<BluetoothDeathRecipient>(this);
}

ndk::ScopedAStatus BluetoothHci::initialize(
    const std::shared_ptr<IBluetoothHciCallbacks>& cb) {
  ALOGI(__func__);

  mFd = open(mDevPath.c_str(), O_RDWR);
  if (mFd < 0) {
    ALOGE("Could not connect to bt: %s (%s)", mDevPath.c_str(),
          strerror(errno));
    return ndk::ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
  }
  if (int ret = SetTerminalRaw(mFd) < 0) {
    ALOGE("Could not make %s a raw terminal %d(%s)", mDevPath.c_str(), ret,
          strerror(errno));
    return ndk::ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
  }

  mCb = cb;
  if (mCb == nullptr) {
    ALOGE("cb == nullptr! -> Unable to call initializationComplete(ERR)");
    return ndk::ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
  }

  mDeathRecipient->LinkToDeath(mCb);

  auto init_ret = cb->initializationComplete(Status::SUCCESS);
  if (!init_ret.isOk()) {
    if (!mDeathRecipient->getHasDied()) {
      ALOGE("Error sending init callback, but no death notification.");
    }
    return ndk::ScopedAStatus::fromServiceSpecificError(
        STATUS_FAILED_TRANSACTION);
  }
  mH4 = std::make_shared<H4Protocol>(
      mFd,
      [](const std::vector<uint8_t>& /* raw_command */) {
        LOG_ALWAYS_FATAL("Unexpected command!");
      },
      [this](const std::vector<uint8_t>& raw_event) {
        mCb->hciEventReceived(raw_event);
      },
      [this](const std::vector<uint8_t>& raw_acl) {
        mCb->hciEventReceived(raw_acl);
      },
      [this](const std::vector<uint8_t>& raw_sco) {
        mCb->hciEventReceived(raw_sco);
      },
      [this](const std::vector<uint8_t>& raw_iso) {
        mCb->hciEventReceived(raw_iso);
      },
      [this]() {
        ALOGI("HCI socket device disconnected");
        mFdWatcher.StopWatchingFileDescriptors();
      });
  mFdWatcher.WatchFdForNonBlockingReads(mFd,
                                        [this](int) { mH4->OnDataReady(); });
  return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothHci::close() {
  ALOGI(__func__);
  mFdWatcher.StopWatchingFileDescriptors();
  ::close(mFd);

  return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothHci::sendHciCommand(
    const std::vector<uint8_t>& packet) {
  send(PacketType::COMMAND, packet);
  return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothHci::sendAclData(
    const std::vector<uint8_t>& packet) {
  send(PacketType::ACL_DATA, packet);
  return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothHci::sendScoData(
    const std::vector<uint8_t>& packet) {
  send(PacketType::SCO_DATA, packet);
  return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothHci::sendIsoData(
    const std::vector<uint8_t>& packet) {
  send(PacketType::ISO_DATA, packet);
  return ndk::ScopedAStatus::ok();
}

void BluetoothHci::send(PacketType type, const std::vector<uint8_t>& v) {
  mH4->Send(type, v);
}

}  // namespace aidl::android::hardware::bluetooth::impl
