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

GeneralResult<OperandType> unvalidatedConvert(const hal::V1_3::OperandType& operandType);
GeneralResult<OperationType> unvalidatedConvert(const hal::V1_3::OperationType& operationType);
GeneralResult<Priority> unvalidatedConvert(const hal::V1_3::Priority& priority);
GeneralResult<Capabilities> unvalidatedConvert(const hal::V1_3::Capabilities& capabilities);
GeneralResult<Capabilities::OperandPerformance> unvalidatedConvert(
        const hal::V1_3::Capabilities::OperandPerformance& operandPerformance);
GeneralResult<Operation> unvalidatedConvert(const hal::V1_3::Operation& operation);
GeneralResult<Operand::LifeTime> unvalidatedConvert(
        const hal::V1_3::OperandLifeTime& operandLifeTime);
GeneralResult<Operand> unvalidatedConvert(const hal::V1_3::Operand& operand);
GeneralResult<Model> unvalidatedConvert(const hal::V1_3::Model& model);
GeneralResult<Model::Subgraph> unvalidatedConvert(const hal::V1_3::Subgraph& subgraph);
GeneralResult<BufferDesc> unvalidatedConvert(const hal::V1_3::BufferDesc& bufferDesc);
GeneralResult<BufferRole> unvalidatedConvert(const hal::V1_3::BufferRole& bufferRole);
GeneralResult<Request> unvalidatedConvert(const hal::V1_3::Request& request);
GeneralResult<Request::MemoryPool> unvalidatedConvert(
        const hal::V1_3::Request::MemoryPool& memoryPool);
GeneralResult<OptionalTimePoint> unvalidatedConvert(
        const hal::V1_3::OptionalTimePoint& optionalTimePoint);
GeneralResult<OptionalDuration> unvalidatedConvert(
        const hal::V1_3::OptionalTimeoutDuration& optionalTimeoutDuration);
GeneralResult<ErrorStatus> unvalidatedConvert(const hal::V1_3::ErrorStatus& errorStatus);

GeneralResult<Priority> convert(const hal::V1_3::Priority& priority);
GeneralResult<Capabilities> convert(const hal::V1_3::Capabilities& capabilities);
GeneralResult<Model> convert(const hal::V1_3::Model& model);
GeneralResult<BufferDesc> convert(const hal::V1_3::BufferDesc& bufferDesc);
GeneralResult<Request> convert(const hal::V1_3::Request& request);
GeneralResult<OptionalTimePoint> convert(const hal::V1_3::OptionalTimePoint& optionalTimePoint);
GeneralResult<OptionalDuration> convert(
        const hal::V1_3::OptionalTimeoutDuration& optionalTimeoutDuration);
GeneralResult<ErrorStatus> convert(const hal::V1_3::ErrorStatus& errorStatus);

GeneralResult<SharedHandle> convert(const hardware::hidl_handle& handle);
GeneralResult<std::vector<BufferRole>> convert(
        const hardware::hidl_vec<hal::V1_3::BufferRole>& bufferRoles);

}  // namespace android::nn

namespace android::hardware::neuralnetworks::V1_3::utils {

nn::GeneralResult<OperandType> unvalidatedConvert(const nn::OperandType& operandType);
nn::GeneralResult<OperationType> unvalidatedConvert(const nn::OperationType& operationType);
nn::GeneralResult<Priority> unvalidatedConvert(const nn::Priority& priority);
nn::GeneralResult<Capabilities> unvalidatedConvert(const nn::Capabilities& capabilities);
nn::GeneralResult<Capabilities::OperandPerformance> unvalidatedConvert(
        const nn::Capabilities::OperandPerformance& operandPerformance);
nn::GeneralResult<Operation> unvalidatedConvert(const nn::Operation& operation);
nn::GeneralResult<OperandLifeTime> unvalidatedConvert(const nn::Operand::LifeTime& operandLifeTime);
nn::GeneralResult<Operand> unvalidatedConvert(const nn::Operand& operand);
nn::GeneralResult<Model> unvalidatedConvert(const nn::Model& model);
nn::GeneralResult<Subgraph> unvalidatedConvert(const nn::Model::Subgraph& subgraph);
nn::GeneralResult<BufferDesc> unvalidatedConvert(const nn::BufferDesc& bufferDesc);
nn::GeneralResult<BufferRole> unvalidatedConvert(const nn::BufferRole& bufferRole);
nn::GeneralResult<Request> unvalidatedConvert(const nn::Request& request);
nn::GeneralResult<Request::MemoryPool> unvalidatedConvert(
        const nn::Request::MemoryPool& memoryPool);
nn::GeneralResult<OptionalTimePoint> unvalidatedConvert(
        const nn::OptionalTimePoint& optionalTimePoint);
nn::GeneralResult<OptionalTimeoutDuration> unvalidatedConvert(
        const nn::OptionalDuration& optionalTimeoutDuration);
nn::GeneralResult<ErrorStatus> unvalidatedConvert(const nn::ErrorStatus& errorStatus);

nn::GeneralResult<Priority> convert(const nn::Priority& priority);
nn::GeneralResult<Capabilities> convert(const nn::Capabilities& capabilities);
nn::GeneralResult<Model> convert(const nn::Model& model);
nn::GeneralResult<BufferDesc> convert(const nn::BufferDesc& bufferDesc);
nn::GeneralResult<Request> convert(const nn::Request& request);
nn::GeneralResult<OptionalTimePoint> convert(const nn::OptionalTimePoint& optionalTimePoint);
nn::GeneralResult<OptionalTimeoutDuration> convert(
        const nn::OptionalDuration& optionalTimeoutDuration);
nn::GeneralResult<ErrorStatus> convert(const nn::ErrorStatus& errorStatus);

nn::GeneralResult<hidl_handle> convert(const nn::SharedHandle& handle);
nn::GeneralResult<hidl_memory> convert(const nn::SharedMemory& memory);
nn::GeneralResult<hidl_vec<BufferRole>> convert(const std::vector<nn::BufferRole>& bufferRoles);

nn::GeneralResult<V1_0::DeviceStatus> convert(const nn::DeviceStatus& deviceStatus);
nn::GeneralResult<V1_1::ExecutionPreference> convert(
        const nn::ExecutionPreference& executionPreference);
nn::GeneralResult<hidl_vec<V1_2::Extension>> convert(const std::vector<nn::Extension>& extensions);
nn::GeneralResult<hidl_vec<hidl_handle>> convert(const std::vector<nn::SharedHandle>& handles);
nn::GeneralResult<hidl_vec<V1_2::OutputShape>> convert(
        const std::vector<nn::OutputShape>& outputShapes);
nn::GeneralResult<V1_2::DeviceType> convert(const nn::DeviceType& deviceType);
nn::GeneralResult<V1_2::MeasureTiming> convert(const nn::MeasureTiming& measureTiming);
nn::GeneralResult<V1_2::Timing> convert(const nn::Timing& timing);

}  // namespace android::hardware::neuralnetworks::V1_3::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_3_CONVERSIONS_H
