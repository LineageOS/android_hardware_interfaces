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

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <signal.h>

#include "wifi.h"
#include "wifi_feature_flags.h"
#include "wifi_legacy_hal.h"
#include "wifi_legacy_hal_factory.h"
#include "wifi_mode_controller.h"

using aidl::android::hardware::wifi::feature_flags::WifiFeatureFlags;
using aidl::android::hardware::wifi::legacy_hal::WifiLegacyHal;
using aidl::android::hardware::wifi::legacy_hal::WifiLegacyHalFactory;
using aidl::android::hardware::wifi::mode_controller::WifiModeController;

#ifdef LAZY_SERVICE
const bool kLazyService = true;
#else
const bool kLazyService = false;
#endif

int main(int /*argc*/, char** argv) {
    signal(SIGPIPE, SIG_IGN);
    android::base::InitLogging(argv, android::base::LogdLogger(android::base::SYSTEM));
    LOG(INFO) << "Wifi Hal is booting up...";

    // Prepare the RPC-serving thread pool. Allocate 1 thread in the pool,
    // which our main thread will join below.
    ABinderProcess_setThreadPoolMaxThreadCount(1);

    const auto iface_tool = std::make_shared<::android::wifi_system::InterfaceTool>();
    const auto legacy_hal_factory = std::make_shared<WifiLegacyHalFactory>(iface_tool);

    // Setup binder service
    std::shared_ptr<aidl::android::hardware::wifi::Wifi> service =
            ndk::SharedRefBase::make<aidl::android::hardware::wifi::Wifi>(
                    iface_tool, legacy_hal_factory, std::make_shared<WifiModeController>(),
                    std::make_shared<WifiFeatureFlags>());
    std::string instance =
            std::string() + aidl::android::hardware::wifi::Wifi::descriptor + "/default";
    if (kLazyService) {
        auto result =
                AServiceManager_registerLazyService(service->asBinder().get(), instance.c_str());
        CHECK_EQ(result, STATUS_OK) << "Failed to register lazy wifi HAL";
    } else {
        auto result = AServiceManager_addService(service->asBinder().get(), instance.c_str());
        CHECK_EQ(result, STATUS_OK) << "Failed to register wifi HAL";
    }

    ABinderProcess_startThreadPool();
    LOG(INFO) << "Joining RPC thread pool";
    ABinderProcess_joinThreadPool();

    LOG(INFO) << "Wifi Hal is terminating...";
    return 0;
}
