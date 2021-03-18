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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_2_CONVERSIONS_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_2_CONVERSIONS_H

#include <android/hardware/neuralnetworks/1.2/types.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/CommonUtils.h>

namespace android::nn {

GeneralResult<OperandType> unvalidatedConvert(const hal::V1_2::OperandType& operandType);
GeneralResult<OperationType> unvalidatedConvert(const hal::V1_2::OperationType& operationType);
GeneralResult<DeviceType> unvalidatedConvert(const hal::V1_2::DeviceType& deviceType);
GeneralResult<Capabilities> unvalidatedConvert(const hal::V1_2::Capabilities& capabilities);
GeneralResult<Capabilities::OperandPerformance> unvalidatedConvert(
        const hal::V1_2::Capabilities::OperandPerformance& operandPerformance);
GeneralResult<Operation> unvalidatedConvert(const hal::V1_2::Operation& operation);
GeneralResult<Operand::SymmPerChannelQuantParams> unvalidatedConvert(
        const hal::V1_2::SymmPerChannelQuantParams& symmPerChannelQuantParams);
GeneralResult<Operand> unvalidatedConvert(const hal::V1_2::Operand& operand);
GeneralResult<Operand::ExtraParams> unvalidatedConvert(
        const hal::V1_2::Operand::ExtraParams& extraParams);
GeneralResult<Model> unvalidatedConvert(const hal::V1_2::Model& model);
GeneralResult<Model::ExtensionNameAndPrefix> unvalidatedConvert(
        const hal::V1_2::Model::ExtensionNameAndPrefix& extensionNameAndPrefix);
GeneralResult<OutputShape> unvalidatedConvert(const hal::V1_2::OutputShape& outputShape);
GeneralResult<MeasureTiming> unvalidatedConvert(const hal::V1_2::MeasureTiming& measureTiming);
GeneralResult<Timing> unvalidatedConvert(const hal::V1_2::Timing& timing);
GeneralResult<Extension> unvalidatedConvert(const hal::V1_2::Extension& extension);
GeneralResult<Extension::OperandTypeInformation> unvalidatedConvert(
        const hal::V1_2::Extension::OperandTypeInformation& operandTypeInformation);
GeneralResult<SharedHandle> unvalidatedConvert(const hardware::hidl_handle& handle);

GeneralResult<DeviceType> convert(const hal::V1_2::DeviceType& deviceType);
GeneralResult<Capabilities> convert(const hal::V1_2::Capabilities& capabilities);
GeneralResult<Model> convert(const hal::V1_2::Model& model);
GeneralResult<MeasureTiming> convert(const hal::V1_2::MeasureTiming& measureTiming);
GeneralResult<Timing> convert(const hal::V1_2::Timing& timing);
GeneralResult<SharedMemory> convert(const hardware::hidl_memory& memory);

GeneralResult<std::vector<Extension>> convert(
        const hardware::hidl_vec<hal::V1_2::Extension>& extensions);
GeneralResult<std::vector<SharedHandle>> convert(
        const hardware::hidl_vec<hardware::hidl_handle>& handles);
GeneralResult<std::vector<OutputShape>> convert(
        const hardware::hidl_vec<hal::V1_2::OutputShape>& outputShapes);

}  // namespace android::nn

namespace android::hardware::neuralnetworks::V1_2::utils {

nn::GeneralResult<OperandType> unvalidatedConvert(const nn::OperandType& operandType);
nn::GeneralResult<OperationType> unvalidatedConvert(const nn::OperationType& operationType);
nn::GeneralResult<DeviceType> unvalidatedConvert(const nn::DeviceType& deviceType);
nn::GeneralResult<Capabilities> unvalidatedConvert(const nn::Capabilities& capabilities);
nn::GeneralResult<Capabilities::OperandPerformance> unvalidatedConvert(
        const nn::Capabilities::OperandPerformance& operandPerformance);
nn::GeneralResult<Operation> unvalidatedConvert(const nn::Operation& operation);
nn::GeneralResult<SymmPerChannelQuantParams> unvalidatedConvert(
        const nn::Operand::SymmPerChannelQuantParams& symmPerChannelQuantParams);
nn::GeneralResult<Operand> unvalidatedConvert(const nn::Operand& operand);
nn::GeneralResult<Operand::ExtraParams> unvalidatedConvert(
        const nn::Operand::ExtraParams& extraParams);
nn::GeneralResult<Model> unvalidatedConvert(const nn::Model& model);
nn::GeneralResult<Model::ExtensionNameAndPrefix> unvalidatedConvert(
        const nn::Model::ExtensionNameAndPrefix& extensionNameAndPrefix);
nn::GeneralResult<OutputShape> unvalidatedConvert(const nn::OutputShape& outputShape);
nn::GeneralResult<MeasureTiming> unvalidatedConvert(const nn::MeasureTiming& measureTiming);
nn::GeneralResult<Timing> unvalidatedConvert(const nn::Timing& timing);
nn::GeneralResult<Extension> unvalidatedConvert(const nn::Extension& extension);
nn::GeneralResult<Extension::OperandTypeInformation> unvalidatedConvert(
        const nn::Extension::OperandTypeInformation& operandTypeInformation);
nn::GeneralResult<hidl_handle> unvalidatedConvert(const nn::SharedHandle& handle);

nn::GeneralResult<DeviceType> convert(const nn::DeviceType& deviceType);
nn::GeneralResult<Capabilities> convert(const nn::Capabilities& capabilities);
nn::GeneralResult<Model> convert(const nn::Model& model);
nn::GeneralResult<MeasureTiming> convert(const nn::MeasureTiming& measureTiming);
nn::GeneralResult<Timing> convert(const nn::Timing& timing);

nn::GeneralResult<hidl_vec<Extension>> convert(const std::vector<nn::Extension>& extensions);
nn::GeneralResult<hidl_vec<hidl_handle>> convert(const std::vector<nn::SharedHandle>& handles);
nn::GeneralResult<hidl_vec<OutputShape>> convert(const std::vector<nn::OutputShape>& outputShapes);

nn::GeneralResult<V1_0::DeviceStatus> convert(const nn::DeviceStatus& deviceStatus);
nn::GeneralResult<V1_0::Request> convert(const nn::Request& request);
nn::GeneralResult<V1_0::ErrorStatus> convert(const nn::ErrorStatus& status);
nn::GeneralResult<V1_1::ExecutionPreference> convert(
        const nn::ExecutionPreference& executionPreference);

}  // namespace android::hardware::neuralnetworks::V1_2::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_2_CONVERSIONS_H
