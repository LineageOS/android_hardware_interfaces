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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_0_CONVERSIONS_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_0_CONVERSIONS_H

#include <android/hardware/neuralnetworks/1.0/types.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/CommonUtils.h>

namespace android::nn {

GeneralResult<OperandType> unvalidatedConvert(const hal::V1_0::OperandType& operandType);
GeneralResult<OperationType> unvalidatedConvert(const hal::V1_0::OperationType& operationType);
GeneralResult<Operand::LifeTime> unvalidatedConvert(const hal::V1_0::OperandLifeTime& lifetime);
GeneralResult<DeviceStatus> unvalidatedConvert(const hal::V1_0::DeviceStatus& deviceStatus);
GeneralResult<Capabilities::PerformanceInfo> unvalidatedConvert(
        const hal::V1_0::PerformanceInfo& performanceInfo);
GeneralResult<Capabilities> unvalidatedConvert(const hal::V1_0::Capabilities& capabilities);
GeneralResult<DataLocation> unvalidatedConvert(const hal::V1_0::DataLocation& location);
GeneralResult<Operand> unvalidatedConvert(const hal::V1_0::Operand& operand);
GeneralResult<Operation> unvalidatedConvert(const hal::V1_0::Operation& operation);
GeneralResult<Model::OperandValues> unvalidatedConvert(
        const hardware::hidl_vec<uint8_t>& operandValues);
GeneralResult<SharedMemory> unvalidatedConvert(const hardware::hidl_memory& memory);
GeneralResult<Model> unvalidatedConvert(const hal::V1_0::Model& model);
GeneralResult<Request::Argument> unvalidatedConvert(
        const hal::V1_0::RequestArgument& requestArgument);
GeneralResult<Request> unvalidatedConvert(const hal::V1_0::Request& request);
GeneralResult<ErrorStatus> unvalidatedConvert(const hal::V1_0::ErrorStatus& status);

GeneralResult<DeviceStatus> convert(const hal::V1_0::DeviceStatus& deviceStatus);
GeneralResult<Capabilities> convert(const hal::V1_0::Capabilities& capabilities);
GeneralResult<Model> convert(const hal::V1_0::Model& model);
GeneralResult<Request> convert(const hal::V1_0::Request& request);
GeneralResult<ErrorStatus> convert(const hal::V1_0::ErrorStatus& status);

}  // namespace android::nn

namespace android::hardware::neuralnetworks::V1_0::utils {

nn::GeneralResult<OperandType> unvalidatedConvert(const nn::OperandType& operandType);
nn::GeneralResult<OperationType> unvalidatedConvert(const nn::OperationType& operationType);
nn::GeneralResult<OperandLifeTime> unvalidatedConvert(const nn::Operand::LifeTime& lifetime);
nn::GeneralResult<DeviceStatus> unvalidatedConvert(const nn::DeviceStatus& deviceStatus);
nn::GeneralResult<PerformanceInfo> unvalidatedConvert(
        const nn::Capabilities::PerformanceInfo& performanceInfo);
nn::GeneralResult<Capabilities> unvalidatedConvert(const nn::Capabilities& capabilities);
nn::GeneralResult<DataLocation> unvalidatedConvert(const nn::DataLocation& location);
nn::GeneralResult<Operand> unvalidatedConvert(const nn::Operand& operand);
nn::GeneralResult<Operation> unvalidatedConvert(const nn::Operation& operation);
nn::GeneralResult<hidl_vec<uint8_t>> unvalidatedConvert(
        const nn::Model::OperandValues& operandValues);
nn::GeneralResult<hidl_memory> unvalidatedConvert(const nn::SharedMemory& memory);
nn::GeneralResult<Model> unvalidatedConvert(const nn::Model& model);
nn::GeneralResult<RequestArgument> unvalidatedConvert(const nn::Request::Argument& requestArgument);
nn::GeneralResult<hidl_memory> unvalidatedConvert(const nn::Request::MemoryPool& memoryPool);
nn::GeneralResult<Request> unvalidatedConvert(const nn::Request& request);
nn::GeneralResult<ErrorStatus> unvalidatedConvert(const nn::ErrorStatus& status);

nn::GeneralResult<DeviceStatus> convert(const nn::DeviceStatus& deviceStatus);
nn::GeneralResult<Capabilities> convert(const nn::Capabilities& capabilities);
nn::GeneralResult<Model> convert(const nn::Model& model);
nn::GeneralResult<Request> convert(const nn::Request& request);
nn::GeneralResult<ErrorStatus> convert(const nn::ErrorStatus& status);

}  // namespace android::hardware::neuralnetworks::V1_0::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_0_CONVERSIONS_H
