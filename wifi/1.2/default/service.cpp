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
#include <hidl/HidlTransportSupport.h>
#include <utils/Looper.h>
#include <utils/StrongPointer.h>

#ifdef ARCH_ARM_32
#include <hwbinder/ProcessState.h>
#include <cutils/properties.h>
#endif

#include "wifi.h"
#include "wifi_feature_flags.h"
#include "wifi_legacy_hal.h"
#include "wifi_mode_controller.h"

using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::hardware::wifi::V1_2::implementation::feature_flags::
    WifiFeatureFlags;
using android::hardware::wifi::V1_2::implementation::legacy_hal::WifiLegacyHal;
using android::hardware::wifi::V1_2::implementation::mode_controller::
    WifiModeController;

#ifdef ARCH_ARM_32
#define DEFAULT_WIFIHAL_HW_BINDER_SIZE_KB 4
size_t getHWBinderMmapSize() {
    size_t value = 0;
    value = property_get_int32("persist.vendor.wifi.wifihal.hw.binder.size", DEFAULT_WIFIHAL_HW_BINDER_SIZE_KB);
    if (!value) value = DEFAULT_WIFIHAL_HW_BINDER_SIZE_KB; // deafult to 1 page of 4 Kb

    return 1024 * value;
}
#endif /* ARCH_ARM_32 */

int main(int /*argc*/, char** argv) {
#ifdef ARCH_ARM_32
    android::hardware::ProcessState::initWithMmapSize(getHWBinderMmapSize());
#endif /* ARCH_ARM_32 */

    android::base::InitLogging(
        argv, android::base::LogdLogger(android::base::SYSTEM));
    LOG(INFO) << "Wifi Hal is booting up...";

    configureRpcThreadpool(1, true /* callerWillJoin */);

    // Setup hwbinder service
    android::sp<android::hardware::wifi::V1_2::IWifi> service =
        new android::hardware::wifi::V1_2::implementation::Wifi(
            std::make_shared<WifiLegacyHal>(),
            std::make_shared<WifiModeController>(),
            std::make_shared<WifiFeatureFlags>());
    CHECK_EQ(service->registerAsService(), android::NO_ERROR)
        << "Failed to register wifi HAL";

    joinRpcThreadpool();

    LOG(INFO) << "Wifi Hal is terminating...";
    return 0;
}
