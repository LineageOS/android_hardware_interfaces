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

Result<OperandType> convert(const hal::V1_0::OperandType& operandType);
Result<OperationType> convert(const hal::V1_0::OperationType& operationType);
Result<Operand::LifeTime> convert(const hal::V1_0::OperandLifeTime& lifetime);
Result<DeviceStatus> convert(const hal::V1_0::DeviceStatus& deviceStatus);
Result<Capabilities::PerformanceInfo> convert(const hal::V1_0::PerformanceInfo& performanceInfo);
Result<Capabilities> convert(const hal::V1_0::Capabilities& capabilities);
Result<DataLocation> convert(const hal::V1_0::DataLocation& location);
Result<Operand> convert(const hal::V1_0::Operand& operand);
Result<Operation> convert(const hal::V1_0::Operation& operation);
Result<Model::OperandValues> convert(const hardware::hidl_vec<uint8_t>& operandValues);
Result<Memory> convert(const hardware::hidl_memory& memory);
Result<Model> convert(const hal::V1_0::Model& model);
Result<Request::Argument> convert(const hal::V1_0::RequestArgument& requestArgument);
Result<Request> convert(const hal::V1_0::Request& request);
Result<ErrorStatus> convert(const hal::V1_0::ErrorStatus& status);

}  // namespace android::nn

namespace android::hardware::neuralnetworks::V1_0::utils {

nn::Result<OperandType> convert(const nn::OperandType& operandType);
nn::Result<OperationType> convert(const nn::OperationType& operationType);
nn::Result<OperandLifeTime> convert(const nn::Operand::LifeTime& lifetime);
nn::Result<DeviceStatus> convert(const nn::DeviceStatus& deviceStatus);
nn::Result<PerformanceInfo> convert(const nn::Capabilities::PerformanceInfo& performanceInfo);
nn::Result<Capabilities> convert(const nn::Capabilities& capabilities);
nn::Result<DataLocation> convert(const nn::DataLocation& location);
nn::Result<Operand> convert(const nn::Operand& operand);
nn::Result<Operation> convert(const nn::Operation& operation);
nn::Result<hidl_vec<uint8_t>> convert(const nn::Model::OperandValues& operandValues);
nn::Result<hidl_memory> convert(const nn::Memory& memory);
nn::Result<Model> convert(const nn::Model& model);
nn::Result<RequestArgument> convert(const nn::Request::Argument& requestArgument);
nn::Result<hidl_memory> convert(const nn::Request::MemoryPool& memoryPool);
nn::Result<Request> convert(const nn::Request& request);
nn::Result<ErrorStatus> convert(const nn::ErrorStatus& status);

}  // namespace android::hardware::neuralnetworks::V1_0::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_0_CONVERSIONS_H
