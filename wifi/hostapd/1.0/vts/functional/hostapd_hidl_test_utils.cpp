/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include <android-base/logging.h>

#include <android/hidl/manager/1.0/IServiceManager.h>
#include <android/hidl/manager/1.0/IServiceNotification.h>
#include <hidl/HidlTransportSupport.h>

#include <wifi_system/hostapd_manager.h>
#include <wifi_system/interface_tool.h>
#include <wifi_system/supplicant_manager.h>

#include "hostapd_hidl_test_utils.h"
#include "wifi_hidl_test_utils.h"

using ::android::sp;
using ::android::hardware::configureRpcThreadpool;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::joinRpcThreadpool;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::wifi::hostapd::V1_0::HostapdStatus;
using ::android::hardware::wifi::hostapd::V1_0::HostapdStatusCode;
using ::android::hardware::wifi::hostapd::V1_0::IHostapd;
using ::android::hardware::wifi::V1_0::ChipModeId;
using ::android::hardware::wifi::V1_0::IWifiChip;
using ::android::hidl::manager::V1_0::IServiceNotification;
using ::android::wifi_system::HostapdManager;
using ::android::wifi_system::SupplicantManager;

namespace {
// Helper function to initialize the driver and firmware to AP mode
// using the vendor HAL HIDL interface.
void initilializeDriverAndFirmware(const std::string& wifi_instance_name) {
    sp<IWifiChip> wifi_chip = getWifiChip(wifi_instance_name);
    ChipModeId mode_id;
    EXPECT_TRUE(configureChipToSupportIfaceType(
        wifi_chip, ::android::hardware::wifi::V1_0::IfaceType::AP, &mode_id));
}

// Helper function to deinitialize the driver and firmware
// using the vendor HAL HIDL interface.
void deInitilializeDriverAndFirmware(const std::string& wifi_instance_name) {
    stopWifi(wifi_instance_name);
}
}  // namespace

// Utility class to wait for wpa_hostapd's HIDL service registration.
class ServiceNotificationListener : public IServiceNotification {
   public:
    Return<void> onRegistration(const hidl_string& fully_qualified_name,
                                const hidl_string& instance_name,
                                bool pre_existing) override {
        if (pre_existing) {
            return Void();
        }
        std::unique_lock<std::mutex> lock(mutex_);
        registered_.push_back(std::string(fully_qualified_name.c_str()) + "/" +
                              instance_name.c_str());
        lock.unlock();
        condition_.notify_one();
        return Void();
    }

    bool registerForHidlServiceNotifications(const std::string& instance_name) {
        if (!IHostapd::registerForNotifications(instance_name, this)) {
            return false;
        }
        configureRpcThreadpool(2, false);
        return true;
    }

    bool waitForHidlService(uint32_t timeout_in_millis,
                            const std::string& instance_name) {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait_for(lock, std::chrono::milliseconds(timeout_in_millis),
                            [&]() { return registered_.size() >= 1; });
        if (registered_.size() != 1) {
            return false;
        }
        std::string expected_registered =
            std::string(IHostapd::descriptor) + "/" + instance_name;
        if (registered_[0] != expected_registered) {
            LOG(ERROR) << "Expected: " << expected_registered
                       << ", Got: " << registered_[0];
            return false;
        }
        return true;
    }

   private:
    std::vector<std::string> registered_{};
    std::mutex mutex_;
    std::condition_variable condition_;
};

void stopSupplicantIfNeeded(const std::string& instance_name) {
    SupplicantManager supplicant_manager;
    if (supplicant_manager.IsSupplicantRunning()) {
        LOG(INFO) << "Supplicant is running, stop supplicant first.";
        ASSERT_TRUE(supplicant_manager.StopSupplicant());
        deInitilializeDriverAndFirmware(instance_name);
        ASSERT_FALSE(supplicant_manager.IsSupplicantRunning());
    }
}

void stopHostapd(const std::string& instance_name) {
    HostapdManager hostapd_manager;

    ASSERT_TRUE(hostapd_manager.StopHostapd());
    deInitilializeDriverAndFirmware(instance_name);
}

void startHostapdAndWaitForHidlService(
    const std::string& wifi_instance_name,
    const std::string& hostapd_instance_name) {
    initilializeDriverAndFirmware(wifi_instance_name);

    android::sp<ServiceNotificationListener> notification_listener =
        new ServiceNotificationListener();
    ASSERT_TRUE(notification_listener->registerForHidlServiceNotifications(
        hostapd_instance_name));

    HostapdManager hostapd_manager;
    ASSERT_TRUE(hostapd_manager.StartHostapd());

    ASSERT_TRUE(
        notification_listener->waitForHidlService(500, hostapd_instance_name));
}

bool is_1_1(const sp<IHostapd>& hostapd) {
    sp<::android::hardware::wifi::hostapd::V1_1::IHostapd> hostapd_1_1 =
        ::android::hardware::wifi::hostapd::V1_1::IHostapd::castFrom(hostapd);
    return hostapd_1_1.get() != nullptr;
}
