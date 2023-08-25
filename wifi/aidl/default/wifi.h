/*
 * Copyright (C) 2022 The Android Open Source Project
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

#ifndef WIFI_H_
#define WIFI_H_

#include <aidl/android/hardware/wifi/BnWifi.h>
#include <android-base/macros.h>
#include <utils/Looper.h>

#include <functional>

#include "aidl_callback_util.h"
#include "wifi_chip.h"
#include "wifi_feature_flags.h"
#include "wifi_legacy_hal.h"
#include "wifi_legacy_hal_factory.h"
#include "wifi_mode_controller.h"

namespace aidl {
namespace android {
namespace hardware {
namespace wifi {

/**
 * Root AIDL interface object used to control the Wifi HAL.
 */
class Wifi : public BnWifi {
  public:
    Wifi(const std::shared_ptr<::android::wifi_system::InterfaceTool> iface_tool,
         const std::shared_ptr<legacy_hal::WifiLegacyHalFactory> legacy_hal_factory,
         const std::shared_ptr<mode_controller::WifiModeController> mode_controller,
         const std::shared_ptr<feature_flags::WifiFeatureFlags> feature_flags);

    bool isValid();

    // AIDL methods exposed.
    ndk::ScopedAStatus registerEventCallback(
            const std::shared_ptr<IWifiEventCallback>& in_callback) override;
    ndk::ScopedAStatus isStarted(bool* _aidl_return) override;
    ndk::ScopedAStatus start() override;
    ndk::ScopedAStatus stop() override;
    ndk::ScopedAStatus getChipIds(std::vector<int32_t>* _aidl_return) override;
    ndk::ScopedAStatus getChip(int32_t in_chipId,
                               std::shared_ptr<IWifiChip>* _aidl_return) override;
    binder_status_t dump(int fd, const char** args, uint32_t numArgs) override;

  private:
    enum class RunState { STOPPED, STARTED, STOPPING };

    // Corresponding worker functions for the AIDL methods.
    ndk::ScopedAStatus registerEventCallbackInternal(
            const std::shared_ptr<IWifiEventCallback>& event_callback __unused);
    ndk::ScopedAStatus startInternal();
    ndk::ScopedAStatus stopInternal(std::unique_lock<std::recursive_mutex>* lock);
    std::pair<std::vector<int32_t>, ndk::ScopedAStatus> getChipIdsInternal();
    std::pair<std::shared_ptr<IWifiChip>, ndk::ScopedAStatus> getChipInternal(int32_t chip_id);

    ndk::ScopedAStatus initializeModeControllerAndLegacyHal();
    ndk::ScopedAStatus stopLegacyHalAndDeinitializeModeController(
            std::unique_lock<std::recursive_mutex>* lock);
    int32_t getChipIdFromWifiChip(std::shared_ptr<WifiChip>& chip);

    // Instance is created in this root level |IWifi| AIDL interface object
    // and shared with all the child AIDL interface objects.
    std::shared_ptr<::android::wifi_system::InterfaceTool> iface_tool_;
    std::shared_ptr<legacy_hal::WifiLegacyHalFactory> legacy_hal_factory_;
    std::shared_ptr<mode_controller::WifiModeController> mode_controller_;
    std::vector<std::shared_ptr<legacy_hal::WifiLegacyHal>> legacy_hals_;
    std::shared_ptr<feature_flags::WifiFeatureFlags> feature_flags_;
    RunState run_state_;
    std::vector<std::shared_ptr<WifiChip>> chips_;
    aidl_callback_util::AidlCallbackHandler<IWifiEventCallback> event_cb_handler_;

    DISALLOW_COPY_AND_ASSIGN(Wifi);
};

}  // namespace wifi
}  // namespace hardware
}  // namespace android
}  // namespace aidl

#endif  // WIFI_H_
