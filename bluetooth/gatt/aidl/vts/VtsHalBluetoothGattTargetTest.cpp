/*
 * Copyright (C) 2025 The Android Open Source Project
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

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/bluetooth/gatt/BnBluetoothGattCallback.h>
#include <aidl/android/hardware/bluetooth/gatt/IBluetoothGatt.h>
#include <aidl/android/hardware/bluetooth/gatt/IBluetoothGattCallback.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <binder/IServiceManager.h>
#include <gmock/gmock.h>
#include <utils/Log.h>

#include <functional>
#include <future>

using ::aidl::android::hardware::bluetooth::gatt::BnBluetoothGattCallback;
using ::aidl::android::hardware::bluetooth::gatt::GattCapabilities;
using ::aidl::android::hardware::bluetooth::gatt::GattCharacteristic;
using ::aidl::android::hardware::bluetooth::gatt::IBluetoothGatt;
using ::aidl::android::hardware::bluetooth::gatt::IBluetoothGattCallback;
using ::aidl::android::hardware::bluetooth::gatt::Uuid;
using ::ndk::ScopedAStatus;
using ::testing::_;

namespace {
constexpr static int kCallbackTimeoutMs = 5000;
constexpr int32_t kGattPropertyNotify = 0x10;
}  // namespace

class MockBluetoothGattCallback : public BnBluetoothGattCallback {
 public:
  MOCK_METHOD(ScopedAStatus, registerServiceComplete,
              (int32_t in_sessionId, IBluetoothGattCallback::Status in_status,
               const std::string& in_reason),
              (override));
  MOCK_METHOD(ScopedAStatus, unregisterServiceComplete,
              (int32_t in_sessionId, const std::string& in_reason), (override));
  MOCK_METHOD(ScopedAStatus, clearServicesComplete,
              (int32_t in_acl_connection_handle, const std::string& in_reason),
              (override));
  MOCK_METHOD(ScopedAStatus, errorReport,
              (int32_t in_acl_connection_handle, int32_t in_local_cid,
               IBluetoothGattCallback::Error in_error,
               const std::string& in_reason),
              (override));
};

class BluetoothGattTest : public ::testing::TestWithParam<std::string> {
 public:
  virtual void SetUp() override {
    ALOGI("SetUp Gatt Test");
    bluetooth_gatt_ = IBluetoothGatt::fromBinder(
        ndk::SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
    ASSERT_NE(bluetooth_gatt_, nullptr);
  }

  virtual void TearDown() override {
    ALOGI("TearDown Gatt Test");
    bluetooth_gatt_ = nullptr;
    ASSERT_EQ(bluetooth_gatt_, nullptr);
  }

  void SetUpCapability() {
    ScopedAStatus status =
        bluetooth_gatt_->getGattCapabilities(&gatt_capabilities_);
    ASSERT_TRUE(status.isOk());
  }

  void RegisterService(IBluetoothGatt::Role role);

  std::shared_ptr<IBluetoothGatt> bluetooth_gatt_;
  GattCapabilities gatt_capabilities_;
};

TEST_P(BluetoothGattTest, init) {
  auto callback = ndk::SharedRefBase::make<MockBluetoothGattCallback>();
  EXPECT_CALL(*callback, registerServiceComplete(_, _, _)).Times(0);
  EXPECT_CALL(*callback, unregisterServiceComplete(_, _)).Times(0);
  EXPECT_CALL(*callback, clearServicesComplete(_, _)).Times(0);
  EXPECT_CALL(*callback, errorReport(_, _, _, _)).Times(0);
  ScopedAStatus status = bluetooth_gatt_->init(callback);
  ASSERT_TRUE(status.isOk());
}

TEST_P(BluetoothGattTest, GetGattCapabilities) {
  SetUpCapability();
  if (gatt_capabilities_.supportedGattClientProperties) {
    // When gatt client is supported, the mandatory property must be supported.
    ASSERT_TRUE((gatt_capabilities_.supportedGattClientProperties &
                 kGattPropertyNotify) != 0);
  }
  if (gatt_capabilities_.supportedGattServerProperties) {
    // When gatt server is supported, the mandatory property must be supported.
    ASSERT_TRUE((gatt_capabilities_.supportedGattServerProperties &
                 kGattPropertyNotify) != 0);
  }
}

TEST_P(BluetoothGattTest, RegisterClientService) {
  SetUpCapability();
  if (!gatt_capabilities_.supportedGattClientProperties) {
    GTEST_SKIP() << "Gatt client is not supported";
  }
  RegisterService(IBluetoothGatt::Role::CLIENT);
}

TEST_P(BluetoothGattTest, RegisterServerService) {
  SetUpCapability();
  if (!gatt_capabilities_.supportedGattServerProperties) {
    GTEST_SKIP() << "Gatt server is not supported";
  }
  RegisterService(IBluetoothGatt::Role::SERVER);
}

TEST_P(BluetoothGattTest, UnregisterService) {
  auto old_callback = ndk::SharedRefBase::make<MockBluetoothGattCallback>();
  auto callback = ndk::SharedRefBase::make<MockBluetoothGattCallback>();
  std::promise<void> unregister_service_cb_promise;
  std::future<void> unregister_service_cb_future{
      unregister_service_cb_promise.get_future()};

  EXPECT_CALL(*callback, registerServiceComplete(_, _, _)).Times(0);
  EXPECT_CALL(*callback, unregisterServiceComplete(_, _))
      .Times(testing::AtMost(1))
      .WillOnce([&unregister_service_cb_promise]() {
        unregister_service_cb_promise.set_value();
        return ::ndk::ScopedAStatus::ok();
      });
  EXPECT_CALL(*callback, clearServicesComplete(_, _)).Times(0);
  EXPECT_CALL(*callback, errorReport(_, _, _, _)).Times(0);

  // Subsequent calls to this method must replace the previously registered one.
  bluetooth_gatt_->init(old_callback);
  bluetooth_gatt_->init(callback);
  GattCapabilities gatt_capabilities;
  bluetooth_gatt_->getGattCapabilities(&gatt_capabilities);

  int32_t session_id = 1;
  ScopedAStatus status = bluetooth_gatt_->unregisterService(session_id);
  std::chrono::milliseconds timeout{kCallbackTimeoutMs};
  if (status.isOk()) {
    // If IBluetoothGatt.unregisterService() returns success, the callback
    // BluetoothGattCallback.unregisterServiceComplete() must be called within
    // the timeout.
    EXPECT_EQ(unregister_service_cb_future.wait_for(timeout),
              std::future_status::ready);
  } else {
    // If IBluetoothGatt.unregisterService() returns failure, the callback
    // BluetoothGattCallback.unregisterServiceComplete() must not be called.
    EXPECT_EQ(unregister_service_cb_future.wait_for(timeout),
              std::future_status::timeout);
  }
}

TEST_P(BluetoothGattTest, ClearService) {
  auto old_callback = ndk::SharedRefBase::make<MockBluetoothGattCallback>();
  auto callback = ndk::SharedRefBase::make<MockBluetoothGattCallback>();
  std::promise<void> clear_service_cb_promise;
  std::future<void> clear_service_cb_future{
      clear_service_cb_promise.get_future()};

  EXPECT_CALL(*callback, registerServiceComplete(_, _, _)).Times(0);
  EXPECT_CALL(*callback, unregisterServiceComplete(_, _)).Times(0);
  EXPECT_CALL(*callback, clearServicesComplete(_, _))
      .Times(testing::AtMost(1))
      .WillOnce([&clear_service_cb_promise]() {
        clear_service_cb_promise.set_value();
        return ::ndk::ScopedAStatus::ok();
      });
  EXPECT_CALL(*callback, errorReport(_, _, _, _)).Times(0);

  // Subsequent calls to this method must replace the previously registered one.
  bluetooth_gatt_->init(old_callback);
  bluetooth_gatt_->init(callback);
  GattCapabilities gatt_capabilities;
  bluetooth_gatt_->getGattCapabilities(&gatt_capabilities);

  int32_t acl_connection_handle = 2;
  ScopedAStatus status = bluetooth_gatt_->clearServices(acl_connection_handle);
  std::chrono::milliseconds timeout{kCallbackTimeoutMs};
  if (status.isOk()) {
    // If IBluetoothGatt.clearServices() returns success, the callback
    // BluetoothGattCallback.clearServicesComplete() must be called within
    // the timeout.
    EXPECT_EQ(clear_service_cb_future.wait_for(timeout),
              std::future_status::ready);
  } else {
    // If IBluetoothGatt.clearServices() returns failure, the callback
    // BluetoothGattCallback.clearServicesComplete() must not be called.
    EXPECT_EQ(clear_service_cb_future.wait_for(timeout),
              std::future_status::timeout);
  }
}

void BluetoothGattTest::RegisterService(IBluetoothGatt::Role role) {
  auto old_callback = ndk::SharedRefBase::make<MockBluetoothGattCallback>();
  auto callback = ndk::SharedRefBase::make<MockBluetoothGattCallback>();
  std::promise<void> register_service_cb_promise;
  std::future<void> register_service_cb_future{
      register_service_cb_promise.get_future()};

  EXPECT_CALL(*callback, registerServiceComplete(_, _, _))
      .Times(testing::AtMost(1))
      .WillOnce([&register_service_cb_promise]() {
        register_service_cb_promise.set_value();
        return ::ndk::ScopedAStatus::ok();
      });
  EXPECT_CALL(*callback, unregisterServiceComplete(_, _)).Times(0);
  EXPECT_CALL(*callback, clearServicesComplete(_, _)).Times(0);
  EXPECT_CALL(*callback, errorReport(_, _, _, _)).Times(0);

  // Subsequent calls to this method must replace the previously registered one.
  bluetooth_gatt_->init(old_callback);
  bluetooth_gatt_->init(callback);
  GattCapabilities gatt_capabilities;
  bluetooth_gatt_->getGattCapabilities(&gatt_capabilities);

  int32_t session_id = 1;
  int32_t acl_connection_handle = 2;
  int32_t att_mtu = 100;
  IBluetoothGatt::Role gatt_role = role;
  Uuid service_uuid;
  std::vector<GattCharacteristic> characteristics;
  ::aidl::android::hardware::contexthub::EndpointId endpoint_id;

  ScopedAStatus status = bluetooth_gatt_->registerService(
      session_id, acl_connection_handle, att_mtu, gatt_role, service_uuid,
      characteristics, endpoint_id);
  std::chrono::milliseconds timeout{kCallbackTimeoutMs};
  if (status.isOk()) {
    // If IBluetoothGatt.registerService() returns success, the callback
    // BluetoothGattCallback.registerServiceComplete() must be called within the
    // timeout.
    EXPECT_EQ(register_service_cb_future.wait_for(timeout),
              std::future_status::ready);
  } else {
    // If IBluetoothGatt.registerService() returns failure, the callback
    // BluetoothGattCallback.registerServiceComplete() must not be called.
    EXPECT_EQ(register_service_cb_future.wait_for(timeout),
              std::future_status::timeout);
  }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(BluetoothGattTest);
INSTANTIATE_TEST_SUITE_P(PerInstance, BluetoothGattTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(
                             IBluetoothGatt::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ABinderProcess_startThreadPool();
  int status = RUN_ALL_TESTS();
  ALOGI("Test result = %d", status);
  return status;
}
