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

#include "EvsEnumerator.h"
#include "EvsGlDisplay.h"

#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <utils/Log.h>

#include <unistd.h>

#include <atomic>
#include <cstdlib>
#include <string_view>

namespace {

using ::aidl::android::frameworks::automotive::display::ICarDisplayProxy;
using ::aidl::android::hardware::automotive::evs::implementation::EvsEnumerator;

constexpr std::string_view kDisplayServiceInstanceName = "/default";
constexpr std::string_view kHwInstanceName = "/hw/0";
constexpr int kNumBinderThreads = 1;

}  // namespace

int main() {
    LOG(INFO) << "EVS Hardware Enumerator service is starting";

    const std::string displayServiceInstanceName =
            std::string(ICarDisplayProxy::descriptor) + std::string(kDisplayServiceInstanceName);
    if (!AServiceManager_isDeclared(displayServiceInstanceName.data())) {
        // TODO: We may just want to disable EVS display.
        LOG(ERROR) << displayServiceInstanceName << " is required.";
        return EXIT_FAILURE;
    }

    std::shared_ptr<ICarDisplayProxy> displayService = ICarDisplayProxy::fromBinder(
            ::ndk::SpAIBinder(AServiceManager_waitForService(displayServiceInstanceName.data())));
    if (!displayService) {
        LOG(ERROR) << "Cannot use " << displayServiceInstanceName << ".  Exiting.";
        return EXIT_FAILURE;
    }

    // Register our service -- if somebody is already registered by our name,
    // they will be killed (their thread pool will throw an exception).
    std::shared_ptr<EvsEnumerator> service =
            ndk::SharedRefBase::make<EvsEnumerator>(displayService);
    if (!service) {
        LOG(ERROR) << "Failed to instantiate the service";
        return EXIT_FAILURE;
    }

    const std::string instanceName =
            std::string(EvsEnumerator::descriptor) + std::string(kHwInstanceName);
    auto err = AServiceManager_addService(service->asBinder().get(), instanceName.data());
    if (err != EX_NONE) {
        LOG(ERROR) << "Failed to register " << instanceName << ", exception = " << err;
        return EXIT_FAILURE;
    }

    if (!ABinderProcess_setThreadPoolMaxThreadCount(kNumBinderThreads)) {
        LOG(ERROR) << "Failed to set thread pool";
        return EXIT_FAILURE;
    }

    ABinderProcess_startThreadPool();
    LOG(INFO) << "EVS Hardware Enumerator is ready";

    ABinderProcess_joinThreadPool();
    // In normal operation, we don't expect the thread pool to exit
    LOG(INFO) << "EVS Hardware Enumerator is shutting down";
    return EXIT_SUCCESS;
}
