/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include "Service.h"

#include <AndroidVersionUtil.h>
#include <android/binder_auto_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <nnapi/IDevice.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/ResilientDevice.h>
#include <string>

#include "Device.h"

namespace aidl::android::hardware::neuralnetworks::utils {

nn::GeneralResult<nn::SharedDevice> getDevice(const std::string& instanceName) {
    auto fullName = std::string(IDevice::descriptor) + "/" + instanceName;
    hal::utils::ResilientDevice::Factory makeDevice =
            [instanceName,
             name = std::move(fullName)](bool blocking) -> nn::GeneralResult<nn::SharedDevice> {
        std::add_pointer_t<AIBinder*(const char*)> getService;
        if (blocking) {
            if (__builtin_available(android __NNAPI_AIDL_MIN_ANDROID_API__, *)) {
                getService = AServiceManager_waitForService;
            } else {
                getService = AServiceManager_getService;
            }
        } else {
            getService = AServiceManager_checkService;
        }

        auto service = IDevice::fromBinder(ndk::SpAIBinder(getService(name.c_str())));
        if (service == nullptr) {
            return NN_ERROR()
                   << (blocking ? "AServiceManager_waitForService (or AServiceManager_getService)"
                                : "AServiceManager_checkService")
                   << " returned nullptr";
        }
        ABinderProcess_startThreadPool();
        return Device::create(instanceName, std::move(service));
    };

    return hal::utils::ResilientDevice::create(std::move(makeDevice));
}

}  // namespace aidl::android::hardware::neuralnetworks::utils
