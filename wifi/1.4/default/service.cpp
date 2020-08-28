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
#include <hidl/HidlLazyUtils.h>
#include <hidl/HidlTransportSupport.h>
#include <utils/Looper.h>
#include <utils/StrongPointer.h>

#include "wifi.h"
#include "wifi_feature_flags.h"
#include "wifi_legacy_hal.h"
#include "wifi_mode_controller.h"

using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::hardware::LazyServiceRegistrar;
using android::hardware::wifi::V1_4::implementation::feature_flags::
    WifiFeatureFlags;
using android::hardware::wifi::V1_4::implementation::iface_util::WifiIfaceUtil;
using android::hardware::wifi::V1_4::implementation::legacy_hal::WifiLegacyHal;
using android::hardware::wifi::V1_4::implementation::mode_controller::
    WifiModeController;

#ifdef LAZY_SERVICE
const bool kLazyService = true;
#else
const bool kLazyService = false;
#endif

int main(int /*argc*/, char** argv) {
    android::base::InitLogging(
        argv, android::base::LogdLogger(android::base::SYSTEM));
    LOG(INFO) << "Wifi Hal is booting up...";

    configureRpcThreadpool(1, true /* callerWillJoin */);

    const auto iface_tool =
        std::make_shared<android::wifi_system::InterfaceTool>();
    // Setup hwbinder service
    android::sp<android::hardware::wifi::V1_4::IWifi> service =
        new android::hardware::wifi::V1_4::implementation::Wifi(
            iface_tool, std::make_shared<WifiLegacyHal>(iface_tool),
            std::make_shared<WifiModeController>(),
            std::make_shared<WifiIfaceUtil>(iface_tool),
            std::make_shared<WifiFeatureFlags>());
    if (kLazyService) {
        auto registrar = LazyServiceRegistrar::getInstance();
        CHECK_EQ(registrar.registerService(service), android::NO_ERROR)
            << "Failed to register wifi HAL";
    } else {
        CHECK_EQ(service->registerAsService(), android::NO_ERROR)
            << "Failed to register wifi HAL";
    }

    joinRpcThreadpool();

    LOG(INFO) << "Wifi Hal is terminating...";
    return 0;
}
