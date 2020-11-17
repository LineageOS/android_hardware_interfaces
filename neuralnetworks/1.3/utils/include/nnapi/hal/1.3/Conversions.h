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

GeneralResult<OperandType> convert(const hal::V1_3::OperandType& operandType);
GeneralResult<OperationType> convert(const hal::V1_3::OperationType& operationType);
GeneralResult<Priority> convert(const hal::V1_3::Priority& priority);
GeneralResult<Capabilities> convert(const hal::V1_3::Capabilities& capabilities);
GeneralResult<Capabilities::OperandPerformance> convert(
        const hal::V1_3::Capabilities::OperandPerformance& operandPerformance);
GeneralResult<Operation> convert(const hal::V1_3::Operation& operation);
GeneralResult<Operand::LifeTime> convert(const hal::V1_3::OperandLifeTime& operandLifeTime);
GeneralResult<Operand> convert(const hal::V1_3::Operand& operand);
GeneralResult<Model> convert(const hal::V1_3::Model& model);
GeneralResult<Model::Subgraph> convert(const hal::V1_3::Subgraph& subgraph);
GeneralResult<BufferDesc> convert(const hal::V1_3::BufferDesc& bufferDesc);
GeneralResult<BufferRole> convert(const hal::V1_3::BufferRole& bufferRole);
GeneralResult<Request> convert(const hal::V1_3::Request& request);
GeneralResult<Request::MemoryPool> convert(const hal::V1_3::Request::MemoryPool& memoryPool);
GeneralResult<OptionalTimePoint> convert(const hal::V1_3::OptionalTimePoint& optionalTimePoint);
GeneralResult<OptionalTimeoutDuration> convert(
        const hal::V1_3::OptionalTimeoutDuration& optionalTimeoutDuration);
GeneralResult<ErrorStatus> convert(const hal::V1_3::ErrorStatus& errorStatus);

GeneralResult<std::vector<BufferRole>> convert(
        const hardware::hidl_vec<hal::V1_3::BufferRole>& bufferRoles);

}  // namespace android::nn

namespace android::hardware::neuralnetworks::V1_3::utils {

nn::GeneralResult<OperandType> convert(const nn::OperandType& operandType);
nn::GeneralResult<OperationType> convert(const nn::OperationType& operationType);
nn::GeneralResult<Priority> convert(const nn::Priority& priority);
nn::GeneralResult<Capabilities> convert(const nn::Capabilities& capabilities);
nn::GeneralResult<Capabilities::OperandPerformance> convert(
        const nn::Capabilities::OperandPerformance& operandPerformance);
nn::GeneralResult<Operation> convert(const nn::Operation& operation);
nn::GeneralResult<OperandLifeTime> convert(const nn::Operand::LifeTime& operandLifeTime);
nn::GeneralResult<Operand> convert(const nn::Operand& operand);
nn::GeneralResult<Model> convert(const nn::Model& model);
nn::GeneralResult<Subgraph> convert(const nn::Model::Subgraph& subgraph);
nn::GeneralResult<BufferDesc> convert(const nn::BufferDesc& bufferDesc);
nn::GeneralResult<BufferRole> convert(const nn::BufferRole& bufferRole);
nn::GeneralResult<Request> convert(const nn::Request& request);
nn::GeneralResult<Request::MemoryPool> convert(const nn::Request::MemoryPool& memoryPool);
nn::GeneralResult<OptionalTimePoint> convert(const nn::OptionalTimePoint& optionalTimePoint);
nn::GeneralResult<OptionalTimeoutDuration> convert(
        const nn::OptionalTimeoutDuration& optionalTimeoutDuration);
nn::GeneralResult<ErrorStatus> convert(const nn::ErrorStatus& errorStatus);

nn::GeneralResult<hidl_vec<BufferRole>> convert(const std::vector<nn::BufferRole>& bufferRoles);

}  // namespace android::hardware::neuralnetworks::V1_3::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_3_CONVERSIONS_H
