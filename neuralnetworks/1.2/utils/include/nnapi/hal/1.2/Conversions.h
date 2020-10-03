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

Result<OperandType> convert(const hal::V1_2::OperandType& operandType);
Result<OperationType> convert(const hal::V1_2::OperationType& operationType);
Result<DeviceType> convert(const hal::V1_2::DeviceType& deviceType);
Result<Capabilities> convert(const hal::V1_2::Capabilities& capabilities);
Result<Capabilities::OperandPerformance> convert(
        const hal::V1_2::Capabilities::OperandPerformance& operandPerformance);
Result<Operation> convert(const hal::V1_2::Operation& operation);
Result<Operand::SymmPerChannelQuantParams> convert(
        const hal::V1_2::SymmPerChannelQuantParams& symmPerChannelQuantParams);
Result<Operand> convert(const hal::V1_2::Operand& operand);
Result<Operand::ExtraParams> convert(const hal::V1_2::Operand::ExtraParams& extraParams);
Result<Model> convert(const hal::V1_2::Model& model);
Result<Model::ExtensionNameAndPrefix> convert(
        const hal::V1_2::Model::ExtensionNameAndPrefix& extensionNameAndPrefix);
Result<OutputShape> convert(const hal::V1_2::OutputShape& outputShape);
Result<MeasureTiming> convert(const hal::V1_2::MeasureTiming& measureTiming);
Result<Timing> convert(const hal::V1_2::Timing& timing);
Result<Extension> convert(const hal::V1_2::Extension& extension);
Result<Extension::OperandTypeInformation> convert(
        const hal::V1_2::Extension::OperandTypeInformation& operandTypeInformation);
Result<NativeHandle> convert(const hardware::hidl_handle& handle);

Result<std::vector<Extension>> convert(const hardware::hidl_vec<hal::V1_2::Extension>& extensions);
Result<std::vector<NativeHandle>> convert(const hardware::hidl_vec<hardware::hidl_handle>& handles);
Result<std::vector<OutputShape>> convert(
        const hardware::hidl_vec<hal::V1_2::OutputShape>& outputShapes);

}  // namespace android::nn

namespace android::hardware::neuralnetworks::V1_2::utils {

nn::Result<OperandType> convert(const nn::OperandType& operandType);
nn::Result<OperationType> convert(const nn::OperationType& operationType);
nn::Result<DeviceType> convert(const nn::DeviceType& deviceType);
nn::Result<Capabilities> convert(const nn::Capabilities& capabilities);
nn::Result<Capabilities::OperandPerformance> convert(
        const nn::Capabilities::OperandPerformance& operandPerformance);
nn::Result<Operation> convert(const nn::Operation& operation);
nn::Result<SymmPerChannelQuantParams> convert(
        const nn::Operand::SymmPerChannelQuantParams& symmPerChannelQuantParams);
nn::Result<Operand> convert(const nn::Operand& operand);
nn::Result<Operand::ExtraParams> convert(const nn::Operand::ExtraParams& extraParams);
nn::Result<Model> convert(const nn::Model& model);
nn::Result<Model::ExtensionNameAndPrefix> convert(
        const nn::Model::ExtensionNameAndPrefix& extensionNameAndPrefix);
nn::Result<OutputShape> convert(const nn::OutputShape& outputShape);
nn::Result<MeasureTiming> convert(const nn::MeasureTiming& measureTiming);
nn::Result<Timing> convert(const nn::Timing& timing);
nn::Result<Extension> convert(const nn::Extension& extension);
nn::Result<Extension::OperandTypeInformation> convert(
        const nn::Extension::OperandTypeInformation& operandTypeInformation);
nn::Result<hidl_handle> convert(const nn::NativeHandle& handle);

nn::Result<hidl_vec<Extension>> convert(const std::vector<nn::Extension>& extensions);
nn::Result<hidl_vec<hidl_handle>> convert(const std::vector<nn::NativeHandle>& handles);
nn::Result<hidl_vec<OutputShape>> convert(const std::vector<nn::OutputShape>& outputShapes);

}  // namespace android::hardware::neuralnetworks::V1_2::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_2_CONVERSIONS_H
