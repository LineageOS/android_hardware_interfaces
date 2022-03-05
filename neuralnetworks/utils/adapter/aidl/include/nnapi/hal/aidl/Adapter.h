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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_AIDL_ADAPTER_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_AIDL_ADAPTER_H

#include <aidl/android/hardware/neuralnetworks/BnDevice.h>
#include <nnapi/IDevice.h>
#include <nnapi/Types.h>

#include <functional>
#include <memory>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on AIDL interface
// lifetimes across processes and for protecting asynchronous calls across AIDL.

namespace aidl::android::hardware::neuralnetworks::adapter {

/**
 * A self-contained unit of work to be executed.
 */
using Task = std::function<void()>;

/**
 * A type-erased executor which executes a task asynchronously.
 *
 * This executor is also provided an optional deadline for when the caller expects is the upper
 * bound for the amount of time to complete the task. If needed, the Executor can retrieve the
 * Application ID (Android User ID) by calling AIBinder_getCallingUid in android/binder_ibinder.h.
 */
using Executor = std::function<void(Task, ::android::nn::OptionalTimePoint)>;

/**
 * Adapt an NNAPI canonical interface object to a AIDL NN HAL interface object.
 *
 * @param device NNAPI canonical IDevice interface object to be adapted.
 * @param executor Type-erased executor to handle executing tasks asynchronously.
 * @return AIDL NN HAL IDevice interface object.
 */
std::shared_ptr<BnDevice> adapt(::android::nn::SharedDevice device, Executor executor);

/**
 * Adapt an NNAPI canonical interface object to a AIDL NN HAL interface object.
 *
 * This function uses a default executor, which will execute tasks from a detached thread.
 *
 * @param device NNAPI canonical IDevice interface object to be adapted.
 * @return AIDL NN HAL IDevice interface object.
 */
std::shared_ptr<BnDevice> adapt(::android::nn::SharedDevice device);

}  // namespace aidl::android::hardware::neuralnetworks::adapter

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_AIDL_ADAPTER_H
