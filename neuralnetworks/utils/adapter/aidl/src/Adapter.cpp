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

#include "Adapter.h"

#include "Device.h"

#include <aidl/android/hardware/neuralnetworks/BnDevice.h>
#include <android/binder_interface_utils.h>
#include <nnapi/IDevice.h>
#include <nnapi/Types.h>

#include <functional>
#include <memory>
#include <thread>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on AIDL interface
// lifetimes across processes and for protecting asynchronous calls across AIDL.

namespace aidl::android::hardware::neuralnetworks::adapter {

std::shared_ptr<BnDevice> adapt(::android::nn::SharedDevice device, Executor executor) {
    return ndk::SharedRefBase::make<Device>(std::move(device), std::move(executor));
}

std::shared_ptr<BnDevice> adapt(::android::nn::SharedDevice device) {
    Executor defaultExecutor = [](Task task, ::android::nn::OptionalTimePoint /*deadline*/) {
        std::thread(std::move(task)).detach();
    };
    return adapt(std::move(device), std::move(defaultExecutor));
}

}  // namespace aidl::android::hardware::neuralnetworks::adapter
