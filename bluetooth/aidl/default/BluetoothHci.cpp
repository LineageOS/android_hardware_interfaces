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

// TODO: Remove custom logging defines from PDL packets.
#undef LOG_INFO
#undef LOG_DEBUG
#include "hci/hci_packets.h"

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
using aidl::android::hardware::bluetooth::Status;

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
    {
      std::lock_guard<std::mutex> guard(mHasDiedMutex);
      has_died_ = true;
    }
    mHci->close();
  }
  BluetoothHci* mHci;
  std::shared_ptr<IBluetoothHciCallbacks> mCb;
  AIBinder_DeathRecipient* clientDeathRecipient_;
  bool getHasDied() {
    std::lock_guard<std::mutex> guard(mHasDiedMutex);
    return has_died_;
  }

 private:
  std::mutex mHasDiedMutex;
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

int BluetoothHci::getFdFromDevPath() {
  int fd = open(mDevPath.c_str(), O_RDWR);
  if (fd < 0) {
    ALOGE("Could not connect to bt: %s (%s)", mDevPath.c_str(),
          strerror(errno));
    return fd;
  }
  if (int ret = SetTerminalRaw(fd) < 0) {
    ALOGI("Could not make %s a raw terminal %d(%s)", mDevPath.c_str(), ret,
          strerror(errno));
  }
  return fd;
}

void BluetoothHci::reset() {
  // Send a reset command and wait until the command complete comes back.

  std::vector<uint8_t> reset;
  ::bluetooth::packet::BitInserter bi{reset};
  ::bluetooth::hci::ResetBuilder::Create()->Serialize(bi);

  auto resetPromise = std::make_shared<std::promise<void>>();
  auto resetFuture = resetPromise->get_future();

  mH4 = std::make_shared<H4Protocol>(
      mFd,
      [](const std::vector<uint8_t>& raw_command) {
        ALOGI("Discarding %d bytes with command type",
              static_cast<int>(raw_command.size()));
      },
      [](const std::vector<uint8_t>& raw_acl) {
        ALOGI("Discarding %d bytes with acl type",
              static_cast<int>(raw_acl.size()));
      },
      [](const std::vector<uint8_t>& raw_sco) {
        ALOGI("Discarding %d bytes with sco type",
              static_cast<int>(raw_sco.size()));
      },
      [resetPromise](const std::vector<uint8_t>& raw_event) {
        bool valid = ::bluetooth::hci::ResetCompleteView::Create(
                         ::bluetooth::hci::CommandCompleteView::Create(
                             ::bluetooth::hci::EventView::Create(
                                 ::bluetooth::hci::PacketView<true>(
                                     std::make_shared<std::vector<uint8_t>>(
                                         raw_event)))))
                         .IsValid();
        if (valid) {
          resetPromise->set_value();
        } else {
          ALOGI("Discarding %d bytes with event type",
                static_cast<int>(raw_event.size()));
        }
      },
      [](const std::vector<uint8_t>& raw_iso) {
        ALOGI("Discarding %d bytes with iso type",
              static_cast<int>(raw_iso.size()));
      },
      [this]() {
        ALOGI("HCI socket device disconnected while waiting for reset");
        mFdWatcher.StopWatchingFileDescriptors();
      });
  mFdWatcher.WatchFdForNonBlockingReads(mFd,
                                        [this](int) { mH4->OnDataReady(); });

  ndk::ScopedAStatus result = send(PacketType::COMMAND, reset);
  if (!result.isOk()) {
    ALOGE("Error sending reset command");
  }
  auto status = resetFuture.wait_for(std::chrono::seconds(1));
  mFdWatcher.StopWatchingFileDescriptors();
  if (status == std::future_status::ready) {
    ALOGI("HCI Reset successful");
  } else {
    ALOGE("HCI Reset Response not received in one second");
  }

  resetPromise.reset();
}

ndk::ScopedAStatus BluetoothHci::initialize(
    const std::shared_ptr<IBluetoothHciCallbacks>& cb) {
  ALOGI(__func__);

  if (cb == nullptr) {
    ALOGE("cb == nullptr! -> Unable to call initializationComplete(ERR)");
    return ndk::ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
  }

  HalState old_state = HalState::READY;
  {
    std::lock_guard<std::mutex> guard(mStateMutex);
    if (mState != HalState::READY) {
      old_state = mState;
    } else {
      mState = HalState::INITIALIZING;
    }
  }

  if (old_state != HalState::READY) {
    ALOGE("initialize: Unexpected State %d", static_cast<int>(old_state));
    close();
    cb->initializationComplete(Status::ALREADY_INITIALIZED);
    return ndk::ScopedAStatus::ok();
  }

  mCb = cb;
  management_.reset(new NetBluetoothMgmt);
  mFd = management_->openHci();
  if (mFd < 0) {
    management_.reset();

    ALOGI("Unable to open Linux interface, trying default path.");
    mFd = getFdFromDevPath();
    if (mFd < 0) {
      mState = HalState::READY;
      cb->initializationComplete(Status::UNABLE_TO_OPEN_INTERFACE);
      return ndk::ScopedAStatus::ok();
    }
  }

  mDeathRecipient->LinkToDeath(mCb);

  // TODO: This should not be necessary when the device implements rfkill.
  reset();

  mH4 = std::make_shared<H4Protocol>(
      mFd,
      [](const std::vector<uint8_t>& /* raw_command */) {
        LOG_ALWAYS_FATAL("Unexpected command!");
      },
      [this](const std::vector<uint8_t>& raw_acl) {
        mCb->aclDataReceived(raw_acl);
      },
      [this](const std::vector<uint8_t>& raw_sco) {
        mCb->scoDataReceived(raw_sco);
      },
      [this](const std::vector<uint8_t>& raw_event) {
        mCb->hciEventReceived(raw_event);
      },
      [this](const std::vector<uint8_t>& raw_iso) {
        mCb->isoDataReceived(raw_iso);
      },
      [this]() {
        ALOGI("HCI socket device disconnected");
        mFdWatcher.StopWatchingFileDescriptors();
      });
  mFdWatcher.WatchFdForNonBlockingReads(mFd,
                                        [this](int) { mH4->OnDataReady(); });

  {
    std::lock_guard<std::mutex> guard(mStateMutex);
    mState = HalState::ONE_CLIENT;
  }
  ALOGI("initialization complete");
  auto status = mCb->initializationComplete(Status::SUCCESS);
  if (!status.isOk()) {
    if (!mDeathRecipient->getHasDied()) {
      ALOGE("Error sending init callback, but no death notification");
    }
    close();
    return ndk::ScopedAStatus::fromServiceSpecificError(
        STATUS_FAILED_TRANSACTION);
  }

  return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothHci::close() {
  ALOGI(__func__);
  {
    std::lock_guard<std::mutex> guard(mStateMutex);
    if (mState != HalState::ONE_CLIENT) {
      ASSERT(mState != HalState::INITIALIZING);
      ALOGI("Already closed");
      return ndk::ScopedAStatus::ok();
    }
    mState = HalState::CLOSING;
  }

  mFdWatcher.StopWatchingFileDescriptors();

  if (management_) {
    management_->closeHci();
  } else {
    ::close(mFd);
  }

  {
    std::lock_guard<std::mutex> guard(mStateMutex);
    mState = HalState::READY;
  }
  return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothHci::sendHciCommand(
    const std::vector<uint8_t>& packet) {
  return send(PacketType::COMMAND, packet);
}

ndk::ScopedAStatus BluetoothHci::sendAclData(
    const std::vector<uint8_t>& packet) {
  return send(PacketType::ACL_DATA, packet);
}

ndk::ScopedAStatus BluetoothHci::sendScoData(
    const std::vector<uint8_t>& packet) {
  return send(PacketType::SCO_DATA, packet);
}

ndk::ScopedAStatus BluetoothHci::sendIsoData(
    const std::vector<uint8_t>& packet) {
  return send(PacketType::ISO_DATA, packet);
}

ndk::ScopedAStatus BluetoothHci::send(PacketType type,
    const std::vector<uint8_t>& v) {
  if (mH4 == nullptr) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
  }
  if (v.empty()) {
    ALOGE("Packet is empty, no data was found to be sent");
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
  }
  mH4->Send(type, v);
  return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::bluetooth::impl
