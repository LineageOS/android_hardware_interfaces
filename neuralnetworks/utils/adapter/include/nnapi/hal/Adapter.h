/*
 * Copyright (C) 2020 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_ADAPTER_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_ADAPTER_H

#include <android/hardware/neuralnetworks/1.3/IDevice.h>
#include <nnapi/IDevice.h>
#include <nnapi/Types.h>
#include <sys/types.h>
#include <functional>
#include <memory>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on HIDL interface
// lifetimes across processes and for protecting asynchronous calls across HIDL.

namespace android::hardware::neuralnetworks::adapter {

/**
 * A self-contained unit of work to be executed.
 */
using Task = std::function<void()>;

/**
 * A type-erased executor which executes a task asynchronously.
 *
 * This executor is also provided with an Application ID (Android User ID) and an optional deadline
 * for when the caller expects is the upper bound for the amount of time to complete the task.
 */
using Executor = std::function<void(Task, uid_t, nn::OptionalTimePoint)>;

/**
 * Adapt an NNAPI canonical interface object to a HIDL NN HAL interface object.
 *
 * The IPreparedModel object created from IDevice::prepareModel or IDevice::preparedModelFromCache
 * must return "const nn::Model*" from IPreparedModel::getUnderlyingResource().
 *
 * @param device NNAPI canonical IDevice interface object to be adapted.
 * @param executor Type-erased executor to handle executing tasks asynchronously.
 * @return HIDL NN HAL IDevice interface object.
 */
sp<V1_3::IDevice> adapt(nn::SharedDevice device, Executor executor);

/**
 * Adapt an NNAPI canonical interface object to a HIDL NN HAL interface object.
 *
 * The IPreparedModel object created from IDevice::prepareModel or IDevice::preparedModelFromCache
 * must return "const nn::Model*" from IPreparedModel::getUnderlyingResource().
 *
 * This function uses a default executor, which will execute tasks from a detached thread.
 *
 * @param device NNAPI canonical IDevice interface object to be adapted.
 * @return HIDL NN HAL IDevice interface object.
 */
sp<V1_3::IDevice> adapt(nn::SharedDevice device);

}  // namespace android::hardware::neuralnetworks::adapter

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_ADAPTER_H
