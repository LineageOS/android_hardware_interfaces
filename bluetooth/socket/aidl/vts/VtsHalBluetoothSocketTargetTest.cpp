/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/bluetooth/socket/BnBluetoothSocketCallback.h>
#include <aidl/android/hardware/bluetooth/socket/IBluetoothSocket.h>
#include <aidl/android/hardware/bluetooth/socket/IBluetoothSocketCallback.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <binder/IServiceManager.h>
#include <utils/Log.h>

#include <future>

using ::aidl::android::hardware::bluetooth::socket::BnBluetoothSocketCallback;
using ::aidl::android::hardware::bluetooth::socket::IBluetoothSocket;
using ::aidl::android::hardware::bluetooth::socket::SocketCapabilities;
using ::aidl::android::hardware::bluetooth::socket::SocketContext;
using ::ndk::ScopedAStatus;

namespace {
constexpr static int kCallbackTimeoutMs = 250;
constexpr static int kOpenedCallbackTimeoutMs = 5000;
}  // namespace

class BluetoothSocketCallback : public BnBluetoothSocketCallback {
 public:
  BluetoothSocketCallback(
      const std::function<
          void(int64_t in_socketId,
               ::aidl::android::hardware::bluetooth::socket::Status in_status,
               const std::string& in_reason)>& on_hal_opened_complete_cb)
      : on_hal_opened_complete_cb_(on_hal_opened_complete_cb) {}

  ScopedAStatus openedComplete(
      int64_t in_socketId,
      ::aidl::android::hardware::bluetooth::socket::Status in_status,
      const std::string& in_reason) override {
    on_hal_opened_complete_cb_(in_socketId, in_status, in_reason);
    return ::ndk::ScopedAStatus::ok();
  }

  ScopedAStatus close(int64_t /* in_socketId */,
                      const std::string& /* in_reason */) override {
    return ::ndk::ScopedAStatus::ok();
  }

 private:
  std::function<void(
      int64_t in_socketId,
      ::aidl::android::hardware::bluetooth::socket::Status in_status,
      const std::string& in_reason)>
      on_hal_opened_complete_cb_;
};

class BluetoothSocketTest : public ::testing::TestWithParam<std::string> {
 public:
  virtual void SetUp() override {
    ALOGI("SetUp Socket Test");
    bluetooth_socket_ = IBluetoothSocket::fromBinder(
        ndk::SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
    ASSERT_NE(bluetooth_socket_, nullptr);
  }

  virtual void TearDown() override {
    ALOGI("TearDown Socket Test");
    bluetooth_socket_ = nullptr;
    ASSERT_EQ(bluetooth_socket_, nullptr);
  }

  std::shared_ptr<IBluetoothSocket> bluetooth_socket_;
};

TEST_P(BluetoothSocketTest, registerCallback) {
  std::promise<void> open_cb_promise;
  std::future<void> open_cb_future{open_cb_promise.get_future()};
  std::shared_ptr<BluetoothSocketCallback> callback =
      ndk::SharedRefBase::make<BluetoothSocketCallback>(
          [&open_cb_promise](auto /* socket_id */, auto /* status */,
                             auto /* reason */) {
            open_cb_promise.set_value();
          });
  ScopedAStatus status = bluetooth_socket_->registerCallback(callback);
  ASSERT_TRUE(status.isOk());
}

TEST_P(BluetoothSocketTest, GetSocketCapabilities) {
  SocketCapabilities socket_capabilities;
  ScopedAStatus status =
      bluetooth_socket_->getSocketCapabilities(&socket_capabilities);
  ASSERT_TRUE(status.isOk());
  ASSERT_TRUE(socket_capabilities.leCocCapabilities.numberOfSupportedSockets >=
              0);
  if (socket_capabilities.leCocCapabilities.numberOfSupportedSockets) {
    // When LE COC is supported, the local MTU must be configured within the
    // valid range defined in the L2CAP specification.
    ASSERT_TRUE(socket_capabilities.leCocCapabilities.mtu >= 23 &&
                socket_capabilities.leCocCapabilities.mtu <= 65535);
  }
  ASSERT_TRUE(socket_capabilities.rfcommCapabilities.numberOfSupportedSockets >=
              0);
  if (socket_capabilities.rfcommCapabilities.numberOfSupportedSockets) {
    // When RFCOMM is supported, the maximum frame size must be configured
    // within the valid range defined in the RFCOMM specification.
    ASSERT_TRUE(socket_capabilities.rfcommCapabilities.maxFrameSize >= 23 &&
                socket_capabilities.rfcommCapabilities.maxFrameSize <= 32767);
  }
}

TEST_P(BluetoothSocketTest, Opened) {
  std::promise<void> open_cb_promise;
  std::future<void> open_cb_future{open_cb_promise.get_future()};
  std::shared_ptr<BluetoothSocketCallback> callback =
      ndk::SharedRefBase::make<BluetoothSocketCallback>(
          [&open_cb_promise](auto /* socket_id */, auto /* status */,
                             auto /* reason */) {
            open_cb_promise.set_value();
          });
  bluetooth_socket_->registerCallback(callback);
  SocketCapabilities socket_capabilities;
  bluetooth_socket_->getSocketCapabilities(&socket_capabilities);

  SocketContext socket_context;
  ScopedAStatus status = bluetooth_socket_->opened(socket_context);
  std::chrono::milliseconds timeout{kOpenedCallbackTimeoutMs};
  if (status.isOk()) {
    // If IBluetoothSocket.opened() returns success, the callback
    // BluetoothSocketCallback.openedComplete() must be called within the
    // timeout.
    EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);
  } else {
    // If IBluetoothSocket.opened() returns failure, the callback
    // BluetoothSocketCallback.openedComplete() must not be called.
    EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::timeout);
  }
}

TEST_P(BluetoothSocketTest, Closed) {
  std::promise<void> open_cb_promise;
  std::future<void> open_cb_future{open_cb_promise.get_future()};
  std::shared_ptr<BluetoothSocketCallback> callback =
      ndk::SharedRefBase::make<BluetoothSocketCallback>(
          [&open_cb_promise](auto /* socket_id */, auto /* status */,
                             auto /* reason */) {
            open_cb_promise.set_value();
          });
  bluetooth_socket_->registerCallback(callback);
  SocketCapabilities socket_capabilities;
  bluetooth_socket_->getSocketCapabilities(&socket_capabilities);

  long socket_id = 1;
  bluetooth_socket_->closed(socket_id);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(BluetoothSocketTest);
INSTANTIATE_TEST_SUITE_P(PerInstance, BluetoothSocketTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(
                             IBluetoothSocket::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ABinderProcess_startThreadPool();
  int status = RUN_ALL_TESTS();
  ALOGI("Test result = %d", status);
  return status;
}
