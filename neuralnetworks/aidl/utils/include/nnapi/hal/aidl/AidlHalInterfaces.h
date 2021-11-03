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

#ifndef ANDROID_FRAMEWORKS_ML_NN_COMMON_AIDL_HAL_INTERFACES_H
#define ANDROID_FRAMEWORKS_ML_NN_COMMON_AIDL_HAL_INTERFACES_H

#include <aidl/android/hardware/neuralnetworks/BnBuffer.h>
#include <aidl/android/hardware/neuralnetworks/BnBurst.h>
#include <aidl/android/hardware/neuralnetworks/BnDevice.h>
#include <aidl/android/hardware/neuralnetworks/BnFencedExecutionCallback.h>
#include <aidl/android/hardware/neuralnetworks/BnPreparedModel.h>
#include <aidl/android/hardware/neuralnetworks/BnPreparedModelCallback.h>
#include <aidl/android/hardware/neuralnetworks/BufferDesc.h>
#include <aidl/android/hardware/neuralnetworks/BufferRole.h>
#include <aidl/android/hardware/neuralnetworks/Capabilities.h>
#include <aidl/android/hardware/neuralnetworks/DataLocation.h>
#include <aidl/android/hardware/neuralnetworks/DeviceBuffer.h>
#include <aidl/android/hardware/neuralnetworks/DeviceType.h>
#include <aidl/android/hardware/neuralnetworks/ErrorStatus.h>
#include <aidl/android/hardware/neuralnetworks/ExecutionPreference.h>
#include <aidl/android/hardware/neuralnetworks/Extension.h>
#include <aidl/android/hardware/neuralnetworks/ExtensionNameAndPrefix.h>
#include <aidl/android/hardware/neuralnetworks/ExtensionOperandTypeInformation.h>
#include <aidl/android/hardware/neuralnetworks/FusedActivationFunc.h>
#include <aidl/android/hardware/neuralnetworks/IBuffer.h>
#include <aidl/android/hardware/neuralnetworks/IDevice.h>
#include <aidl/android/hardware/neuralnetworks/IFencedExecutionCallback.h>
#include <aidl/android/hardware/neuralnetworks/IPreparedModel.h>
#include <aidl/android/hardware/neuralnetworks/IPreparedModelCallback.h>
#include <aidl/android/hardware/neuralnetworks/IPreparedModelParcel.h>
#include <aidl/android/hardware/neuralnetworks/Memory.h>
#include <aidl/android/hardware/neuralnetworks/Model.h>
#include <aidl/android/hardware/neuralnetworks/NumberOfCacheFiles.h>
#include <aidl/android/hardware/neuralnetworks/Operand.h>
#include <aidl/android/hardware/neuralnetworks/OperandExtraParams.h>
#include <aidl/android/hardware/neuralnetworks/OperandLifeTime.h>
#include <aidl/android/hardware/neuralnetworks/OperandPerformance.h>
#include <aidl/android/hardware/neuralnetworks/OperandType.h>
#include <aidl/android/hardware/neuralnetworks/Operation.h>
#include <aidl/android/hardware/neuralnetworks/OperationType.h>
#include <aidl/android/hardware/neuralnetworks/OutputShape.h>
#include <aidl/android/hardware/neuralnetworks/PerformanceInfo.h>
#include <aidl/android/hardware/neuralnetworks/Priority.h>
#include <aidl/android/hardware/neuralnetworks/Request.h>
#include <aidl/android/hardware/neuralnetworks/RequestArgument.h>
#include <aidl/android/hardware/neuralnetworks/RequestMemoryPool.h>
#include <aidl/android/hardware/neuralnetworks/Subgraph.h>
#include <aidl/android/hardware/neuralnetworks/SymmPerChannelQuantParams.h>
#include <aidl/android/hardware/neuralnetworks/Timing.h>

namespace android::nn {

namespace aidl_hal = ::aidl::android::hardware::neuralnetworks;

inline constexpr aidl_hal::Priority kDefaultPriorityAidl = aidl_hal::Priority::MEDIUM;

}  // namespace android::nn

#endif  // ANDROID_FRAMEWORKS_ML_NN_COMMON_AIDL_HAL_INTERFACES_H
