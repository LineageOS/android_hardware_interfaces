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

GeneralResult<OperandType> convert(const hal::V1_0::OperandType& operandType);
GeneralResult<OperationType> convert(const hal::V1_0::OperationType& operationType);
GeneralResult<Operand::LifeTime> convert(const hal::V1_0::OperandLifeTime& lifetime);
GeneralResult<DeviceStatus> convert(const hal::V1_0::DeviceStatus& deviceStatus);
GeneralResult<Capabilities::PerformanceInfo> convert(
        const hal::V1_0::PerformanceInfo& performanceInfo);
GeneralResult<Capabilities> convert(const hal::V1_0::Capabilities& capabilities);
GeneralResult<DataLocation> convert(const hal::V1_0::DataLocation& location);
GeneralResult<Operand> convert(const hal::V1_0::Operand& operand);
GeneralResult<Operation> convert(const hal::V1_0::Operation& operation);
GeneralResult<Model::OperandValues> convert(const hardware::hidl_vec<uint8_t>& operandValues);
GeneralResult<Memory> convert(const hardware::hidl_memory& memory);
GeneralResult<Model> convert(const hal::V1_0::Model& model);
GeneralResult<Request::Argument> convert(const hal::V1_0::RequestArgument& requestArgument);
GeneralResult<Request> convert(const hal::V1_0::Request& request);
GeneralResult<ErrorStatus> convert(const hal::V1_0::ErrorStatus& status);

}  // namespace android::nn

namespace android::hardware::neuralnetworks::V1_0::utils {

nn::GeneralResult<OperandType> convert(const nn::OperandType& operandType);
nn::GeneralResult<OperationType> convert(const nn::OperationType& operationType);
nn::GeneralResult<OperandLifeTime> convert(const nn::Operand::LifeTime& lifetime);
nn::GeneralResult<DeviceStatus> convert(const nn::DeviceStatus& deviceStatus);
nn::GeneralResult<PerformanceInfo> convert(
        const nn::Capabilities::PerformanceInfo& performanceInfo);
nn::GeneralResult<Capabilities> convert(const nn::Capabilities& capabilities);
nn::GeneralResult<DataLocation> convert(const nn::DataLocation& location);
nn::GeneralResult<Operand> convert(const nn::Operand& operand);
nn::GeneralResult<Operation> convert(const nn::Operation& operation);
nn::GeneralResult<hidl_vec<uint8_t>> convert(const nn::Model::OperandValues& operandValues);
nn::GeneralResult<hidl_memory> convert(const nn::Memory& memory);
nn::GeneralResult<Model> convert(const nn::Model& model);
nn::GeneralResult<RequestArgument> convert(const nn::Request::Argument& requestArgument);
nn::GeneralResult<hidl_memory> convert(const nn::Request::MemoryPool& memoryPool);
nn::GeneralResult<Request> convert(const nn::Request& request);
nn::GeneralResult<ErrorStatus> convert(const nn::ErrorStatus& status);

}  // namespace android::hardware::neuralnetworks::V1_0::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_0_CONVERSIONS_H
