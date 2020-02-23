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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_3_CONVERSIONS_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_3_CONVERSIONS_H

#include <android/hardware/neuralnetworks/1.3/IPreparedModel.h>
#include <android/hardware/neuralnetworks/1.3/types.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/CommonUtils.h>

namespace android::nn {

Result<OperandType> convert(const hal::V1_3::OperandType& operandType);
Result<OperationType> convert(const hal::V1_3::OperationType& operationType);
Result<Priority> convert(const hal::V1_3::Priority& priority);
Result<Capabilities> convert(const hal::V1_3::Capabilities& capabilities);
Result<Capabilities::OperandPerformance> convert(
        const hal::V1_3::Capabilities::OperandPerformance& operandPerformance);
Result<Operation> convert(const hal::V1_3::Operation& operation);
Result<Operand::LifeTime> convert(const hal::V1_3::OperandLifeTime& operandLifeTime);
Result<Operand> convert(const hal::V1_3::Operand& operand);
Result<Model> convert(const hal::V1_3::Model& model);
Result<Model::Subgraph> convert(const hal::V1_3::Subgraph& subgraph);
Result<BufferDesc> convert(const hal::V1_3::BufferDesc& bufferDesc);
Result<BufferRole> convert(const hal::V1_3::BufferRole& bufferRole);
Result<Request> convert(const hal::V1_3::Request& request);
Result<Request::MemoryPool> convert(const hal::V1_3::Request::MemoryPool& memoryPool);
Result<OptionalTimePoint> convert(const hal::V1_3::OptionalTimePoint& optionalTimePoint);
Result<OptionalTimeoutDuration> convert(
        const hal::V1_3::OptionalTimeoutDuration& optionalTimeoutDuration);
Result<ErrorStatus> convert(const hal::V1_3::ErrorStatus& errorStatus);

Result<std::vector<BufferRole>> convert(
        const hardware::hidl_vec<hal::V1_3::BufferRole>& bufferRoles);

}  // namespace android::nn

namespace android::hardware::neuralnetworks::V1_3::utils {

nn::Result<OperandType> convert(const nn::OperandType& operandType);
nn::Result<OperationType> convert(const nn::OperationType& operationType);
nn::Result<Priority> convert(const nn::Priority& priority);
nn::Result<Capabilities> convert(const nn::Capabilities& capabilities);
nn::Result<Capabilities::OperandPerformance> convert(
        const nn::Capabilities::OperandPerformance& operandPerformance);
nn::Result<Operation> convert(const nn::Operation& operation);
nn::Result<OperandLifeTime> convert(const nn::Operand::LifeTime& operandLifeTime);
nn::Result<Operand> convert(const nn::Operand& operand);
nn::Result<Model> convert(const nn::Model& model);
nn::Result<Subgraph> convert(const nn::Model::Subgraph& subgraph);
nn::Result<BufferDesc> convert(const nn::BufferDesc& bufferDesc);
nn::Result<BufferRole> convert(const nn::BufferRole& bufferRole);
nn::Result<Request> convert(const nn::Request& request);
nn::Result<Request::MemoryPool> convert(const nn::Request::MemoryPool& memoryPool);
nn::Result<OptionalTimePoint> convert(const nn::OptionalTimePoint& optionalTimePoint);
nn::Result<OptionalTimeoutDuration> convert(
        const nn::OptionalTimeoutDuration& optionalTimeoutDuration);
nn::Result<ErrorStatus> convert(const nn::ErrorStatus& errorStatus);

nn::Result<hidl_vec<BufferRole>> convert(const std::vector<nn::BufferRole>& bufferRoles);

}  // namespace android::hardware::neuralnetworks::V1_3::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_3_CONVERSIONS_H
